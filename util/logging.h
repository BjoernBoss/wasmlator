#pragma once

#include <ustring/ustring.h>

namespace util {
	/* log the single message */
	void log(const std::u8string_view& msg);

	/* debug-log the single message */
	void debug(const std::u8string_view& msg);

	/* fail the single message */
	void fail [[noreturn]] (const std::u8string_view& msg);

	/* build the message and log it */
	template <class... Args>
	void log(const Args&... args) {
		std::u8string str = str::Build<std::u8string>(args...);
		util::log(std::u8string_view{ str });
	}

	/* build the message and debug-log it */
	template <class... Args>
	void debug(const Args&... args) {
		std::u8string str = str::Build<std::u8string>(args...);
		util::debug(std::u8string_view{ str });
	}

	/* build the message and log it and abort the entire execution */
	template <class... Args>
	void fail [[noreturn]] (const Args&... args) {
		std::u8string str = str::Build<std::u8string>(args...);
		util::fail(std::u8string_view{ str });
	}
}
