#pragma once

#include <ustring/ustring.h>

namespace host {
	enum class LogLevel : uint8_t {
		none,
		log,
		debug
	};
	void SetLogLevel(host::LogLevel level);
	host::LogLevel GetLogLevel();

	/* write message as [log] out */
	void Log(const std::u8string_view& msg);

	/* write message as [debug] out */
	void Debug(const std::u8string_view& msg);

	/* write message as [fatal] out and terminate the execution */
	void Fatal [[noreturn]] (const std::u8string_view& msg);

	/* build the message and log it using host::Log */
	template <class... Args>
	void Log(const Args&... args) {
		/* only build the string if it will be printed */
		if (host::GetLogLevel() >= host::LogLevel::log) {
			std::u8string str = str::Build<std::u8string>(args...);
			host::Log(std::u8string_view{ str });
		}
	}

	/* build the message and log it using host::Debug */
	template <class... Args>
	void Debug(const Args&... args) {
		/* only build the string if it will be printed */
		if (host::GetLogLevel() >= host::LogLevel::debug) {
			std::u8string str = str::Build<std::u8string>(args...);
			host::Debug(std::u8string_view{ str });
		}
	}

	/* build the message and log it using host::Fatal */
	template <class... Args>
	void Fatal [[noreturn]] (const Args&... args) {
		std::u8string str = str::Build<std::u8string>(args...);
		host::Fatal(std::u8string_view{ str });
	}
}
