#pragma once

#include <ustring/ustring.h>
#include <cinttypes>
#include <string>
#include <utility>

namespace util {
	enum class PathState :uint8_t {
		invalid,
		absolute,
		relative
	};

	/* check if the path is well-formed and absolute or relative */
	util::PathState TestPath(std::u8string_view path);

	/* append the relative path to the absolute path and return the new absolute canonicalized path
	*	Note: if [rel] is absolute itself, it will simply be returned */
	std::u8string MergePaths(std::u8string_view abs, std::u8string_view rel);

	/* create a canonicalized version of the path (equivalent to MergePaths(path, "") */
	std::u8string CanonicalPath(std::u8string_view path);

	/* return the path and last name component
	*	Note: the name will not contain any slashes
	*	Note: if the path ends on a slash, the name will be empty
	*	Note: if the path contains at most one leading slash, the first part will be empty */
	std::pair<std::u8string_view, std::u8string_view> SplitName(std::u8string_view path);

	/* return the first path component and the remainder of the path
	*	Note: the first path component will not contain any slashes
	*	Note: if the remainder is not empty, it will start with a slash
	*	Note: if the path contains at most one leading slash, the remainder will remain empty */
	std::pair<std::u8string_view, std::u8string_view> SplitRoot(std::u8string_view path);
}
