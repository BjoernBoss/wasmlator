#include "../system.h"

static util::Logger logger{ u8"sys::Syscall" };

int64_t sys::detail::FileIO::fCheckRead(int64_t fd) const {
	if (fd < 0 || fd >= int64_t(pFiles.size()) || pFiles[fd].node == 0 || !pFiles[fd].read)
		return errCode::eBadFd;
	if (!pFiles[fd].node->directory().empty())
		return errCode::eIsDirectory;
	return 0;
}
int64_t sys::detail::FileIO::fCheckWrite(int64_t fd) const {
	if (fd < 0 || fd >= int64_t(pFiles.size()) || pFiles[fd].node == 0 || !pFiles[fd].write)
		return errCode::eBadFd;
	if (!pFiles[fd].node->directory().empty())
		return errCode::eIsDirectory;
	return 0;
}

int64_t sys::detail::FileIO::fCreateNode(std::u8string_view path, bool follow, bool append, std::function<int64_t(int64_t, detail::FileNode*, const env::FileStats&)> callback) {
	std::unique_ptr<detail::FileNode> node;

	/* check if the terminal has explicitly been created */
	if (path == u8"/dev/tty")
		node.reset(new impl::Terminal());
	else if (path.starts_with(u8"/proc"))
		node.reset(new impl::ProcDirectory(pSyscall, path));
	else {
		logger.error(u8"Unable open file [", path, u8']');
		return callback(errCode::eNoEntry, 0, {});
	}

	/* bind the node */
	size_t index = 0;
	while (index < pNodes.size() && pNodes[index].node.get() != 0)
		++index;
	if (index == pNodes.size())
		pNodes.push_back({ {}, 0 });

	/* setup the new node */
	pNodes[index].user = 1;
	pNodes[index].node = std::move(node);

	/* reset the link-following and configure the node itself (will at all
	*	times be invoked and therfore at all times be cleaned up properly) */
	pLinkFollow = 0;
	return pNodes[index].node->setup([=, this](int64_t result, const env::FileStats& stats) -> int64_t {
		pNodes[index].node->bindConfigure(index, (stats.type == env::FileType::directory ? path : u8""));

		/* check if the opening failed */
		if (result != errCode::eSuccess) {
			fDropNode(pNodes[index].node.get());
			logger.error(u8"Unable open file [", path, u8']');
			return callback(result, 0, stats);
		}

		/* check if symlinks should be followed */
		if (!follow || stats.type != env::FileType::link)
			return callback(result, pNodes[index].node.get(), stats);
		fDropNode(pNodes[index].node.get());

		/* check if the follow-limit has been reached */
		if (++pLinkFollow >= env::MaxFollowSymLinks) {
			logger.error(u8"Max link following for [", path, u8"] reached");
			return callback(errCode::eLoop, 0, stats);
		}
		return fCreateNode(stats.link, true, append, callback);
		});
}
int64_t sys::detail::FileIO::fCreateFile(detail::FileNode* node, bool read, bool write, bool modify, bool closeOnExecute) {
	/* lookup the next free file-descriptor */
	size_t index = 0;
	while (index < pFiles.size() && pFiles[index].node != 0)
		++index;
	if (index == pNodes.size())
		pFiles.push_back({ 0, false, false, false, false });

	/* setup the new file */
	pFiles[index].node = node;
	pFiles[index].read = read;
	pFiles[index].write = write;
	pFiles[index].modify = modify;
	pFiles[index].closeOnExecute = closeOnExecute;

	/* increment the usage-counter of the node and return the new index */
	++pNodes[node->index()].user;
	return index;
}
void sys::detail::FileIO::fDropNode(detail::FileNode* node) {
	if (--pNodes[node->index()].user == 0)
		pNodes[node->index()].node.reset();
}

int64_t sys::detail::FileIO::fOpenAt(int64_t dirfd, std::u8string_view path, uint64_t flags, uint64_t mode) {
	/* validate the dir-fd */
	if (dirfd != detail::fdWDirectory && (dirfd < 0 || uint64_t(dirfd) >= pFiles.size() || pFiles[dirfd].node == 0))
		return errCode::eBadFd;

	/* validate the flags */
	if ((flags & fileFlags::readOnly) != fileFlags::readOnly
		&& (flags & fileFlags::writeOnly) != fileFlags::writeOnly
		&& (flags & fileFlags::readWrite) != fileFlags::readWrite)
		return errCode::eInvalid;
	if ((flags & fileFlags::writeOnly) == fileFlags::writeOnly && (flags & fileFlags::readWrite) == fileFlags::readWrite)
		return errCode::eInvalid;

	/* extract all of the flag bits */
	bool read = ((flags & fileFlags::readWrite) == fileFlags::readWrite) || ((flags & fileFlags::writeOnly) != fileFlags::writeOnly);
	bool write = ((flags & fileFlags::readWrite) == fileFlags::readWrite) || ((flags & fileFlags::writeOnly) == fileFlags::writeOnly);
	bool follow = ((flags & fileFlags::noSymlinkFollow) != fileFlags::noSymlinkFollow);
	bool directory = ((flags & fileFlags::directory) == fileFlags::directory);
	bool openOnly = ((flags & fileFlags::openOnly) == fileFlags::openOnly);
	bool closeOnExecute = ((flags & fileFlags::closeOnExecute) == fileFlags::closeOnExecute);
	bool blocking = ((flags & fileFlags::nonBlocking) != fileFlags::nonBlocking);
	bool append = ((flags & fileFlags::append) == fileFlags::append);
	if (openOnly)
		read = (write = false);

	/* validate the path */
	util::PathState state = util::TestPath(path);
	if (state == util::PathState::invalid)
		return errCode::eInvalid;

	/* construct the actual path */
	if (state == util::PathState::relative && dirfd != detail::fdWDirectory && pFiles[dirfd].node->directory().empty())
		return errCode::eNotDirectory;
	std::u8string actual = util::MergePaths(dirfd == detail::fdWDirectory ? pWDirectory : pFiles[dirfd].node->directory(), path);

	/* create the new file node */
	return fCreateNode(actual, follow, append, [=, this](int64_t result, detail::FileNode* node, const env::FileStats& stats) -> int64_t {
		/* check if the creation failed */
		if (node == 0)
			return result;

		/* check if a symbolic link is being opened */
		if (!follow && stats.type == env::FileType::link && !openOnly) {
			fDropNode(node);
			return errCode::eLoop;
		}

		/* check if the directory-flag matches */
		if (directory && stats.type != env::FileType::directory) {
			fDropNode(node);
			return errCode::eNotDirectory;
		}

		/* check if the node is readable/writable/modifyable */
		if (write && !node->writable()) {
			fDropNode(node);
			return errCode::eReadOnly;
		}

		/* configure the blocking-behavior */
		node->modify(blocking);

		/* register the node as new file and remove this created user */
		result = fCreateFile(node, read, write, !openOnly, closeOnExecute);
		fDropNode(node);
		return result;
		});
}
int64_t sys::detail::FileIO::fRead(detail::FileNode* node, std::function<int64_t(int64_t)> callback) {
	return node->read(pBuffer, callback);
}
int64_t sys::detail::FileIO::fWrite(detail::FileNode* node) const {
	return node->write(pBuffer, [=](int64_t result) -> int64_t { return result; });
}
int64_t sys::detail::FileIO::fReadLinkAt(int64_t dirfd, std::u8string_view path, env::guest_t address, uint64_t size) {
	if (size == 0)
		return errCode::eInvalid;

	/* validate the dir-fd */
	if (dirfd != detail::fdWDirectory && (dirfd < 0 || uint64_t(dirfd) >= pFiles.size() || pFiles[dirfd].node == 0))
		return errCode::eBadFd;

	/* validate the path */
	util::PathState state = util::TestPath(path);
	if (state == util::PathState::invalid)
		return errCode::eInvalid;

	/* construct the actual path */
	if (state == util::PathState::relative && dirfd != detail::fdWDirectory && pFiles[dirfd].node->directory().empty())
		return errCode::eNotDirectory;
	std::u8string actual = util::MergePaths(dirfd == detail::fdWDirectory ? pWDirectory : pFiles[dirfd].node->directory(), path);

	/* create the node and perform the stat-read */
	return fCreateNode(actual, false, false, [=, this](int64_t result, detail::FileNode* node, const env::FileStats& stats) -> int64_t {
		/* check if the creation failed */
		if (node == 0)
			return result;
		fDropNode(node);

		/* check if the destination is a symbolic link */
		if (stats.type != env::FileType::link)
			return errCode::eInvalid;

		/* write the link to the guest */
		result = std::min<int64_t>(size, stats.link.size());
		env::Instance()->memory().mwrite(address, stats.link.data(), uint32_t(result), env::Usage::Write);
		return result;
		});
}

bool sys::detail::FileIO::setup(detail::Syscall* syscall) {
	pSyscall = syscall;

	/* setup the initial open file-entries to the terminal (stdin/stdout/stderr - terminal creation happens inplace) */
	detail::FileNode* terminal = 0;
	fCreateNode(u8"/dev/tty", false, false, [&](int64_t, detail::FileNode* node, const env::FileStats&) -> int64_t { terminal = node; return 0; });
	fCreateFile(terminal, true, false, true, false);
	fCreateFile(terminal, false, true, true, false);
	fCreateFile(terminal, false, true, true, false);
	fDropNode(terminal);

	/* validate the current path */
	std::u8string_view wDirectory = util::SplitPath(pSyscall->config().path).first;
	if (util::TestPath(wDirectory) != util::PathState::absolute) {
		logger.error(u8"Current directory [", wDirectory, u8"] must be a valid absolute path");
		return false;
	}
	pWDirectory = util::CanonicalPath(wDirectory);
	logger.info(u8"Configured with current working directory [", pWDirectory, u8']');
	return true;
}
sys::detail::FileNode* sys::detail::FileIO::link(int64_t fd) {
	if (fd < 0 || fd >= int64_t(pFiles.size()) || pFiles[fd].node == 0)
		return 0;
	++pNodes[pFiles[fd].node->index()].user;
	return pFiles[fd].node;
}
void sys::detail::FileIO::drop(detail::FileNode* node) {
	fDropNode(node);
}

int64_t sys::detail::FileIO::openat(int64_t dirfd, std::u8string_view path, uint64_t flags, uint64_t mode) {
	return fOpenAt(dirfd, path, flags, mode);
}
int64_t sys::detail::FileIO::open(std::u8string_view path, uint64_t flags, uint64_t mode) {
	return fOpenAt(detail::fdWDirectory, path, flags, mode);
}
int64_t sys::detail::FileIO::read(int64_t fd, env::guest_t address, uint64_t size) {
	/* validate the fd and access */
	int64_t res = fCheckRead(fd);
	if (res != 0 || size == 0)
		return res;

	/* fetch the data to be read */
	pBuffer.resize(size);
	return fRead(pFiles[fd].node, [address, this](int64_t read) -> int64_t {
		if (read <= 0)
			return read;

		/* write the data back to the guest memory */
		env::Instance()->memory().mwrite(address, pBuffer.data(), uint32_t(read), env::Usage::Write);
		return read;
		});
}
int64_t sys::detail::FileIO::readv(int64_t fd, env::guest_t vec, uint64_t count) {
	/* validate the fd and access */
	int64_t res = fCheckRead(fd);
	if (res != 0 || count == 0)
		return res;
	pCached.clear();
	pBuffer.clear();

	/* collect the total readable size */
	for (size_t i = 0; i < count; ++i) {
		env::guest_t address = env::Instance()->memory().read<env::guest_t>(vec + (i * 2 + 0) * sizeof(env::guest_t));
		uint64_t size = env::Instance()->memory().read<env::guest_t>(vec + (i * 2 + 1) * sizeof(env::guest_t));
		if (size == 0)
			continue;
		pCached.push_back(address);
		pCached.push_back(size);
		pBuffer.insert(pBuffer.end(), size, 0);
	}

	/* fetch the data to be read */
	return fRead(pFiles[fd].node, [this](int64_t read) -> int64_t {
		if (read <= 0)
			return read;

		/* write the data back to the guest memory */
		int64_t offset = 0;
		for (size_t i = 0; i < pCached.size(); i += 2) {
			size_t actual = std::min<size_t>(pBuffer.size() - offset, pCached[i + 1]);
			env::Instance()->memory().mwrite(pCached[i], pBuffer.data() + offset, uint32_t(actual), env::Usage::Write);

			/* check if the totally read data have been written back */
			offset += actual;
			if (offset >= int64_t(pBuffer.size()))
				break;
		}
		return read;
		});
}
int64_t sys::detail::FileIO::write(int64_t fd, env::guest_t address, uint64_t size) {
	/* validate the fd and access */
	int64_t res = fCheckWrite(fd);
	if (res != 0 || size == 0)
		return res;

	/* read the data from the guest */
	pBuffer.resize(size);
	env::Instance()->memory().mread(pBuffer.data(), address, uint32_t(size), env::Usage::Read);

	/* write the data out */
	return fWrite(pFiles[fd].node);
}
int64_t sys::detail::FileIO::writev(int64_t fd, env::guest_t vec, uint64_t count) {
	/* validate the fd and access */
	int64_t res = fCheckWrite(fd);
	if (res != 0 || count == 0)
		return res;
	pBuffer.clear();
	size_t total = 0;

	/* read the data from the guest */
	for (size_t i = 0; i < count; ++i) {
		/* read the next vector to be written out */
		env::guest_t address = env::Instance()->memory().read<env::guest_t>(vec + (i * 2 + 0) * sizeof(env::guest_t));
		uint64_t size = env::Instance()->memory().read<env::guest_t>(vec + (i * 2 + 1) * sizeof(env::guest_t));
		if (size == 0)
			continue;

		/* collect the data and append them to the buffer */
		pBuffer.insert(pBuffer.end(), size, 0);
		env::Instance()->memory().mread(pBuffer.data() + total, address, uint32_t(size), env::Usage::Read);
		total += size;
	}

	/* write the data out */
	return fWrite(pFiles[fd].node);
}
int64_t sys::detail::FileIO::readlinkat(int64_t dirfd, std::u8string_view path, env::guest_t address, uint64_t size) {
	return fReadLinkAt(dirfd, path, address, size);
}
int64_t sys::detail::FileIO::readlink(std::u8string_view path, env::guest_t address, uint64_t size) {
	return fReadLinkAt(detail::fdWDirectory, path, address, size);
}
