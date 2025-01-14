#include "../system.h"

static util::Logger logger{ u8"sys::Syscall" };

int64_t sys::detail::FileIO::fCheckRead(int64_t fd) const {
	if (fd < 0 || fd >= int64_t(pFiles.size()) || pFiles[fd].state == detail::FileState::none || !pFiles[fd].read)
		return errCode::eBadFd;
	if (pFiles[fd].state == detail::FileState::directory)
		return errCode::eIsDir;
	return 0;
}
int64_t sys::detail::FileIO::fCheckWrite(int64_t fd) const {
	if (fd < 0 || fd >= int64_t(pFiles.size()) || pFiles[fd].state == detail::FileState::none || !pFiles[fd].write)
		return errCode::eBadFd;
	if (pFiles[fd].state == detail::FileState::directory)
		return errCode::eIsDir;
	return 0;
}

int64_t sys::detail::FileIO::fOpenAt(int64_t dirfd, std::u8string_view path, uint64_t flags, uint64_t mode) {
	/* validate the dir-fd */
	if (dirfd != detail::workingDirectoryFD && (dirfd < 0 || uint64_t(dirfd) >= pFiles.size() || pFiles[dirfd].state == detail::FileState::none))
		return errCode::eBadFd;

	/* validate the flags */
	if ((flags & fileFlags::readOnly) != fileFlags::readOnly
		&& (flags & fileFlags::writeOnly) != fileFlags::writeOnly
		&& (flags & fileFlags::readWrite) != fileFlags::readWrite)
		return errCode::eInvalid;
	if ((flags & fileFlags::writeOnly) == fileFlags::writeOnly && (flags & fileFlags::readWrite) == fileFlags::readWrite)
		return errCode::eInvalid;
	bool read = ((flags & fileFlags::readWrite) == fileFlags::readWrite) || ((flags & fileFlags::writeOnly) != fileFlags::writeOnly);
	bool write = ((flags & fileFlags::readWrite) == fileFlags::readWrite) || ((flags & fileFlags::writeOnly) == fileFlags::writeOnly);

	/* validate the path */
	util::PathState state = util::TestPath(path);
	if (state == util::PathState::invalid)
		return errCode::eInvalid;

	/* construct the actual path */
	if (state == util::PathState::relative && dirfd != detail::workingDirectoryFD && pFiles[dirfd].state != detail::FileState::directory)
		return errCode::eNotDirectory;
	std::u8string actual = util::MergePaths(dirfd == detail::workingDirectoryFD ? pCurrent : pFiles[dirfd].path, path);

	/* check if its a special purpose file */
	detail::FileState file = detail::FileState::none;
	if (actual == u8"/dev/stdin")
		file = detail::FileState::stdIn;
	else if (actual == u8"/dev/stdout" || actual == u8"/dev/tty")
		file = detail::FileState::stdOut;
	else if (actual == u8"/dev/stderr")
		file = detail::FileState::errOut;
	if (file == detail::FileState::none) {
		logger.fatal(u8"Currently not implemented (same as non-blocking for /dev/stdin");
		return errCode::eNoEntry;
	}

	/* allocate the next slot (skip the first 3, as they will never be re-assigned) */
	for (size_t i = 3; i < pFiles.size(); ++i) {
		if (pFiles[i].state != detail::FileState::none)
			continue;
		pFiles[i].state = file;
		pFiles[i].path = actual;
		pFiles[i].read = read;
		pFiles[i].write = write;
		return i;
	}
	pFiles.push_back({ actual, file, read, write });
	return pFiles.size() - 1;
}
int64_t sys::detail::FileIO::fRead(detail::FileEntry& entry, std::vector<uint8_t>& buffer) {
	return errCode::eIO;
}
int64_t sys::detail::FileIO::fWrite(detail::FileEntry& entry, const std::vector<uint8_t>& buffer) {
	if (entry.state == detail::FileState::stdOut) {
		host::GuestStd({ reinterpret_cast<const char8_t*>(buffer.data()), buffer.size() });
		return buffer.size();
	}
	if (entry.state == detail::FileState::errOut) {
		host::GuestErr({ reinterpret_cast<const char8_t*>(buffer.data()), buffer.size() });
		return buffer.size();
	}
	return errCode::eIO;
}

bool sys::detail::FileIO::setup(std::u8string_view currentDirectory) {
	/* setup the standard inputs/outputs */
	pFiles.push_back({ u8"/dev/stdin", detail::FileState::stdIn, true, false });
	pFiles.push_back({ u8"/dev/stdout", detail::FileState::stdOut, false, true });
	pFiles.push_back({ u8"/dev/stderr", detail::FileState::errOut, false, true });

	/* validate the current path */
	if (util::TestPath(currentDirectory) != util::PathState::absolute) {
		logger.error(u8"Current directory [", currentDirectory, u8"] must be a valid absolute path");
		return false;
	}
	pCurrent = util::CanonicalPath(currentDirectory);
	logger.info(u8"Configured with current working directory [", pCurrent, u8']');
	return true;
}
int64_t sys::detail::FileIO::openat(int64_t dirfd, std::u8string_view path, uint64_t flags, uint64_t mode) {
	return fOpenAt(dirfd, path, flags, mode);
}
int64_t sys::detail::FileIO::open(std::u8string_view path, uint64_t flags, uint64_t mode) {
	return fOpenAt(detail::workingDirectoryFD, path, flags, mode);
}
int64_t sys::detail::FileIO::read(int64_t fd, env::guest_t address, uint64_t size) {
	/* validate the fd and access */
	int64_t res = fCheckRead(fd);
	if (res != 0 || size == 0)
		return res;

	/* fetch the data to be read */
	std::vector<uint8_t> buffer(size, 0);
	int64_t read = fRead(pFiles[fd], buffer);

	/* write the data out to the guest */
	if (read > 0)
		env::Instance()->memory().mwrite(address, buffer.data(), uint32_t(read), env::Usage::Write);
	return read;
}
int64_t sys::detail::FileIO::readv(int64_t fd, env::guest_t vec, uint64_t count) {
	/* validate the fd and access */
	int64_t res = fCheckRead(fd);
	if (res != 0 || count == 0)
		return res;
	std::vector<uint8_t> buffer;
	std::vector<uint64_t> cached;

	/* collect the total readable size */
	for (size_t i = 0; i < count; ++i) {
		env::guest_t address = env::Instance()->memory().read<env::guest_t>(vec + (i * 2 + 0) * sizeof(env::guest_t));
		uint64_t size = env::Instance()->memory().read<env::guest_t>(vec + (i * 2 + 1) * sizeof(env::guest_t));
		if (size == 0)
			continue;
		cached.push_back(address);
		cached.push_back(size);
		buffer.insert(buffer.end(), size, 0);
	}

	/* read the entire data */
	int64_t read = fRead(pFiles[fd], buffer);
	if (read <= 0)
		return read;

	/* write the data back to the guest memory */
	int64_t offset = 0;
	for (size_t i = 0; i < cached.size(); i += 2) {
		size_t actual = std::min<size_t>(buffer.size() - offset, cached[i + 1]);
		env::Instance()->memory().mwrite(cached[i], buffer.data() + offset, uint32_t(actual), env::Usage::Write);

		/* check if the totally read data have been written back */
		offset += actual;
		if (offset >= int64_t(buffer.size()))
			break;
	}
	return read;
}
int64_t sys::detail::FileIO::write(int64_t fd, env::guest_t address, uint64_t size) {
	/* validate the fd and access */
	int64_t res = fCheckWrite(fd);
	if (res != 0 || size == 0)
		return res;

	/* read the data from the guest */
	std::vector<uint8_t> buffer(size, 0);
	env::Instance()->memory().mread(buffer.data(), address, uint32_t(size), env::Usage::Read);

	/* write the data out */
	return fWrite(pFiles[fd], buffer);
}
int64_t sys::detail::FileIO::writev(int64_t fd, env::guest_t vec, uint64_t count) {
	/* validate the fd and access */
	int64_t res = fCheckWrite(fd);
	if (res != 0 || count == 0)
		return res;
	std::vector<uint8_t> buffer;
	size_t total = 0;

	/* read the data from the guest */
	for (size_t i = 0; i < count; ++i) {
		/* read the next vector to be written out */
		env::guest_t address = env::Instance()->memory().read<env::guest_t>(vec + (i * 2 + 0) * sizeof(env::guest_t));
		uint64_t size = env::Instance()->memory().read<env::guest_t>(vec + (i * 2 + 1) * sizeof(env::guest_t));
		if (size == 0)
			continue;

		/* collect the data and append them to the buffer */
		buffer.insert(buffer.end(), size, 0);
		env::Instance()->memory().mread(buffer.data() + total, address, uint32_t(size), env::Usage::Read);
		total += size;
	}

	/* write the data out */
	return fWrite(pFiles[fd], buffer);
}
