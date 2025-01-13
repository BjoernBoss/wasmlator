#include "../system.h"

static host::Logger logger{ u8"sys::Syscall" };

uint64_t sys::detail::FileIO::fOpenAt(int64_t dirfd, std::u8string_view path, uint64_t flags, uint64_t mode) {
	/* validate the dir-fd */
	if (dirfd != detail::workingDirectoryFD && (dirfd < 0 || uint64_t(dirfd) >= pFiles.size() || pFiles[dirfd].state == detail::FileState::none))
		return uint64_t(-errCode::eBadFd);

	/* validate the flags */
	if ((flags & fileFlags::readOnly) != fileFlags::readOnly
		&& (flags & fileFlags::writeOnly) != fileFlags::writeOnly
		&& (flags & fileFlags::readWrite) != fileFlags::readWrite)
		return uint64_t(-errCode::eInvalid);

	/* validate the path */
	util::PathState state = util::TestPath(path);
	if (state == util::PathState::invalid)
		return uint64_t(-errCode::eInvalid);

	/* construct the actual path */
	if (state == util::PathState::relative && dirfd != detail::workingDirectoryFD && pFiles[dirfd].state != detail::FileState::directory)
		return uint64_t(-errCode::eNotDirectory);
	std::u8string actual = util::MergePaths(dirfd == detail::workingDirectoryFD ? pCurrent : pFiles[dirfd].path, path);

	/* check if its a special purpose file */
	detail::FileState file = detail::FileState::none;
	if (actual == u8"/dev/stdin")
		file = detail::FileState::stdIn;
	else if (actual == u8"/dev/stdout" || actual == u8"/dev/tty")
		file = detail::FileState::stdOut;
	else if (actual == u8"/dev/stderr")
		file = detail::FileState::errOut;
	if (file == detail::FileState::none)
		return uint64_t(-errCode::eNoEntry);

	/* allocate the next slot (skip the first 3, as they will never be re-assigned) */
	for (size_t i = 2; i < pFiles.size(); ++i) {
		if (pFiles[i].state != detail::FileState::none)
			continue;
		pFiles[i].state = file;
		pFiles[i].path = actual;
		return i;
	}
	pFiles.push_back({ actual, file });
	return pFiles.size() - 1;
}

bool sys::detail::FileIO::setup(std::u8string_view currentDirectory) {
	/* setup the standard inputs/outputs */
	pFiles.push_back({ u8"/dev/stdin", detail::FileState::stdIn });
	pFiles.push_back({ u8"/dev/stdout", detail::FileState::stdOut });
	pFiles.push_back({ u8"/dev/stderr", detail::FileState::errOut });

	/* validate the current path */
	if (util::TestPath(currentDirectory) != util::PathState::absolute) {
		logger.error(u8"Current directory [", currentDirectory, u8"] must be a valid absolute path");
		return false;
	}
	pCurrent = util::CanonicalPath(currentDirectory);
	logger.info(u8"Configured with current working directory [", pCurrent, u8']');
	return true;
}
uint64_t sys::detail::FileIO::openat(int64_t dirfd, std::u8string_view path, uint64_t flags, uint64_t mode) {
	return fOpenAt(dirfd, path, flags, mode);
}
uint64_t sys::detail::FileIO::open(std::u8string_view path, uint64_t flags, uint64_t mode) {
	return fOpenAt(detail::workingDirectoryFD, path, flags, mode);
}
