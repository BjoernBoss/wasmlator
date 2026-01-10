/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025-2026 Bjoern Boss Henrichsen */
#include "util-path.h"

util::PathState util::TestPath(std::u8string_view path) {
	if (path.empty())
		return util::PathState::invalid;

	/* validate the encoding */
	if (!str::View{ path }.isValid())
		return util::PathState::invalid;

	/* check if the path starts with a slash */
	return ((path[0] == u8'/' || path[0] == u8'\\') ? util::PathState::absolute : util::PathState::relative);
}

std::u8string util::MergePaths(std::u8string_view a, std::u8string_view b) {
	std::u8string out;
	size_t lastComponent = 0;

	/* check if the seconds path is actually absolute, in which case the first path can be ignored */
	size_t s = 0;
	if (!b.empty() && (b[0] == u8'/' || b[0] == u8'\\'))
		s = 1;

	/* iterate over the two strings and add them to the output */
	for (; s < 2; ++s) {
		std::u8string_view str = (s == 0 ? a : b);

		/* add each character and remove relative path modifiers (i == size is an implicit slash) */
		for (size_t i = 0; i <= str.size(); ++i) {
			/* check if its just a simple character and write it out */
			if (i < str.size() && str[i] != u8'\\' && str[i] != u8'/') {
				out.push_back(str[i]);
				continue;
			}

			/* check if this is a chain of slashes, and skip the last slash */
			if (out.ends_with(u8"/"))
				continue;
			std::u8string_view last = std::u8string_view{ out }.substr(lastComponent);

			/* check if the last component was itself */
			if (last == u8".") {
				out.pop_back();
				continue;
			}

			/* check if the last component was either a normal name or is a root back-traversal */
			if (last != u8".." || lastComponent == 0) {
				out.push_back(u8'/');
				lastComponent = out.size();
				continue;
			}

			/* find the start of the previous component */
			size_t prevComponent = lastComponent - 1;
			while (prevComponent > 0 && out[prevComponent - 1] != u8'/')
				--prevComponent;

			/* check if the previous component is a back-traversal or if its the root or if it can be removed */
			std::u8string_view prev = std::u8string_view{ out }.substr(prevComponent, lastComponent - prevComponent - 1);
			if (prev == u8"..")
				out.push_back(u8'/');
			else if (prev == u8"")
				out.resize(lastComponent);
			else
				out.resize(prevComponent);
			lastComponent = out.size();
		}
	}

	/* check if the path ends on a slash and remove it */
	if (out.back() == u8'/' && out.size() > 1)
		out.pop_back();

	/* check if the path is empty and replace it with a current-directory */
	else if (out.empty())
		out.push_back(u8'.');
	return out;
}

std::u8string util::CanonicalPath(std::u8string_view path) {
	return util::MergePaths(path, u8"");
}

std::u8string util::AbsolutePath(std::u8string_view path) {
	return util::MergePaths(u8"/", path);
}

std::u8string util::CleanPath(std::u8string_view path) {
	std::u8string out;

	for (size_t i = 0; i < path.size(); ++i) {
		/* check if its any component */
		if (path[i] != u8'/' && path[i] != u8'\\') {
			out.push_back(path[i]);
			continue;
		}

		/* check if its a double-slash */
		if (out.ends_with(u8"/"))
			continue;

		/* check if its just self */
		else if (out.ends_with(u8"/.") || out == u8".")
			out.pop_back();
		else
			out.push_back(u8'/');
	}
	return out;
}

std::pair<std::u8string_view, std::u8string_view> util::SplitName(std::u8string_view path) {
	/* find the last slash */
	size_t name = path.size();
	while (name > 0 && path[name - 1] != u8'/' && path[name - 1] != u8'\\')
		--name;

	/* return the two sub-strings */
	if (name <= 1)
		return { u8"/", path.substr(name) };
	return { path.substr(0, (name > 0 ? name - 1 : 0)), path.substr(name) };
}

std::pair<std::u8string_view, std::u8string_view> util::SplitRoot(std::u8string_view path) {
	if (path.empty())
		return { u8"", u8"" };

	/* check if the path starts with a slash, and skip it */
	if (path[0] == u8'/' || path[0] == u8'\\')
		path = path.substr(1);

	/* find the next slash, which separates the two components */
	size_t remainder = 0;
	while (remainder < path.size() && path[remainder] != u8'/' && path[remainder] != u8'\\')
		++remainder;

	/* return the two sub-strings */
	return { path.substr(0, remainder), path.substr(remainder) };
}
