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

	/* return the name component of the path (empty name if ends on '/' or string is empty) */
	std::pair<std::u8string_view, std::u8string_view> SplitPath(std::u8string_view path);
}
