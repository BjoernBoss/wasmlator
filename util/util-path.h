/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025 Bjoern Boss Henrichsen */
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

	/* append path a to path b and return the canonicalized path
	*	Note: if [a] is absolute, it will consume all [..], if [b] is absolute, only it will be returned */
	std::u8string MergePaths(std::u8string_view a, std::u8string_view b);

	/* create a canonicalized version of the path (equivalent to MergePaths(path, u8"")) */
	std::u8string CanonicalPath(std::u8string_view path);

	/* create a canonicalized absolute version of the path (equivalent to MergePaths(u8"/", path)) */
	std::u8string AbsolutePath(std::u8string_view path);

	/* create a canonicalized version of the path but leaving '..' in the path (useful when every path-components still needs to be visited) */
	std::u8string CleanPath(std::u8string_view path);

	/* return the path and last name component
	*	Note: the name will not contain any slashes
	*	Note: if the path ends on a slash, the name will be empty
	*	Note: if the path contains at least one leading slash, the first part will not be empty */
	std::pair<std::u8string_view, std::u8string_view> SplitName(std::u8string_view path);

	/* return the first path component and the remainder of the path
	*	Note: the first path component will not contain any slashes
	*	Note: if the remainder is not empty, it will start with a slash
	*	Note: if the path contains at most one leading slash, the remainder will remain empty */
	std::pair<std::u8string_view, std::u8string_view> SplitRoot(std::u8string_view path);
}
