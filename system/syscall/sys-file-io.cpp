#include "../system.h"

static host::Logger logger{ u8"sys::Syscall" };

sys::detail::PathState sys::detail::FileIO::fCheckPath(std::u8string_view path) const {
	if (path.empty())
		return detail::PathState::invalid;

	/* validate the encoding */
	if (!str::View{ path }.isValid())
		return detail::PathState::invalid;

	/* validate that no two slashes follow each other */
	for (size_t i = 1; i < path.size(); ++i) {
		if (path[i] != u8'/' && path[i] != u8'\\')
			continue;
		if (path[i - 1] == u8'/' || path[i - 1] == u8'\\')
			return detail::PathState::invalid;

	}

	return ((path[0] == u8'/' || path[0] == u8'\\') ? detail::PathState::absolute : detail::PathState::relative);
}
std::u8string sys::detail::FileIO::fMergePaths(std::u8string_view abs, std::u8string_view rel) const {
	std::u8string out;

	/* check if the seconds path is actually absolute, in which case the first path can be ignored */
	size_t s = 0;
	if (rel.starts_with(u8"/"))
		s = 1;

	/* iterate over the two strings and add them to the output */
	for (; s < 2; ++s) {
		std::u8string_view str = (s == 0 ? abs : rel);

		/* add each character and remove relative path modifiers (i == size is an implicit slash) */
		for (size_t i = 0; i <= str.size(); ++i) {
			/* check if its just a simple character and write it out */
			if (i < str.size() && str[i] != u8'\\' && str[i] != u8'/') {
				out.push_back(str[i]);
				continue;
			}

			/* check if the last component was itself */
			if (out.ends_with(u8"/.")) {
				out.pop_back();
				continue;
			}

			/* check if the last component was a back-traversal */
			if (!out.ends_with(u8"/..")) {
				out.push_back(u8'/');
				continue;
			}

			/* check if the path is back at the root */
			if (out.size() == 3) {
				out.erase(out.end() - 2, out.end());
				continue;
			}

			/* pop the last path component */
			out.erase(out.end() - 3, out.end());
			while (out.back() != u8'/')
				out.pop_back();
		}

		/* check if the path ends on a slash, which is only required for the first string */
		if (s == 1) {
			if (out.back() == u8'/')
				out.pop_back();
			break;
		}
		if (out.back() != u8'/')
			out.push_back(u8'/');
	}
	return out;
}

uint64_t sys::detail::FileIO::fOpenAt(int64_t dirfd, std::u8string path, uint64_t flags, uint64_t mode) {
	/* validate the dir-fd */
	if (dirfd != detail::workingDirectoryFD && (dirfd < 0 || uint64_t(dirfd) >= pFiles.size() || pFiles[dirfd].state == detail::FileState::none))
		return uint64_t(-errCode::eBadFd);

	/* validate the flags */
	if ((flags & fileFlags::readOnly) != fileFlags::readOnly
		&& (flags & fileFlags::writeOnly) != fileFlags::writeOnly
		&& (flags & fileFlags::readWrite) != fileFlags::readWrite)
		return uint64_t(-errCode::eInvalid);

	/* validate the path */
	detail::PathState state = fCheckPath(path);
	if (state == detail::PathState::invalid)
		return uint64_t(-errCode::eInvalid);

	/* construct the actual path */
	if (state == detail::PathState::relative && dirfd != detail::workingDirectoryFD && pFiles[dirfd].state != detail::FileState::directory)
		return uint64_t(-errCode::eNotDirectory);
	path = fMergePaths(dirfd == detail::workingDirectoryFD ? pCurrent : pFiles[dirfd].path, path);

	/* check if its a special purpose file */
	detail::FileState file = detail::FileState::none;
	if (path == u8"/dev/stdin")
		file = detail::FileState::stdIn;
	else if (path == u8"/dev/stdout" || path == u8"/dev/tty")
		file = detail::FileState::stdOut;
	else if (path == u8"/dev/stderr")
		file = detail::FileState::errOut;
	if (file == detail::FileState::none)
		return uint64_t(-errCode::eNoEntry);

	/* allocate the next slot (skip the first 3, as they will never be re-assigned) */
	for (size_t i = 2; i < pFiles.size(); ++i) {
		if (pFiles[i].state != detail::FileState::none)
			continue;
		pFiles[i].state = file;
		pFiles[i].path = path;
		return i;
	}
	pFiles.push_back({ path,file });
	return pFiles.size() - 1;
}

bool sys::detail::FileIO::setup(std::u8string currentDirectory) {
	/* setup the standard inputs/outputs */
	pFiles.push_back({ u8"/dev/stdin", detail::FileState::stdIn });
	pFiles.push_back({ u8"/dev/stdout", detail::FileState::stdOut });
	pFiles.push_back({ u8"/dev/stderr", detail::FileState::errOut });

	/* validate the current path */
	if (fCheckPath(currentDirectory) != detail::PathState::absolute) {
		logger.error(u8"Current directory [", currentDirectory, u8"] must be a valid absolute path");
		return false;
	}
	pCurrent = fMergePaths(currentDirectory, u8"");
	logger.info(u8"Configured with current working directory [", pCurrent, u8']');
	return true;
}
uint64_t sys::detail::FileIO::openat(int64_t dirfd, std::u8string path, uint64_t flags, uint64_t mode) {
	return fOpenAt(dirfd, path, flags, mode);
}
uint64_t sys::detail::FileIO::open(std::u8string path, uint64_t flags, uint64_t mode) {
	return fOpenAt(detail::workingDirectoryFD, path, flags, mode);
}
