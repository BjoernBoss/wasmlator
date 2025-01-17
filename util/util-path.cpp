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

std::u8string util::MergePaths(std::u8string_view abs, std::u8string_view rel) {
	/* starting the output with a slash ensures that even a relative 'abs' results in an absolute path */
	std::u8string out{ u8"/" };

	/* check if the seconds path is actually absolute, in which case the first path can be ignored */
	size_t s = 0;
	if (!rel.empty() && (rel[0] == u8'/' || rel[0] == u8'\\'))
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

			/* check if this is a chain of slashes, and skip the last slash */
			if (out.ends_with(u8"/"))
				continue;

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
	}

	/* check if the path ends on a slash and remove it */
	if (out.back() == u8'/' && out.size() > 1)
		out.pop_back();
	return out;
}

std::u8string util::CanonicalPath(std::u8string_view path) {
	return util::MergePaths(path, u8"");
}

std::pair<std::u8string_view, std::u8string_view> util::SplitName(std::u8string_view path) {
	if (path.empty())
		return { u8"", u8"" };

	/* find the last slash */
	size_t name = path.size();
	while (name > 0 && path[name - 1] != u8'/' && path[name != u8'\\'])
		--name;

	/* return the two sub-strings */
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
