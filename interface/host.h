#pragma once

#include <ustring/ustring.h>

namespace host {
	enum class LogLevel : uint8_t {
		none,
		log,
		debug
	};
	void SetLogLevel(host::LogLevel level);

	/* write message as [log] out */
	void Log(const std::u8string_view& msg);

	/* write message as [debug] out */
	void Debug(const std::u8string_view& msg);

	/* write message as [fail] out and terminate the execution */
	void Fail [[noreturn]] (const std::u8string_view& msg);

	/* build the message and log it using host::log */
	template <class... Args>
	void Log(const Args&... args) {
		std::u8string str = str::Build<std::u8string>(args...);
		host::Log(std::u8string_view{ str });
	}

	/* build the message and log it using host::debug */
	template <class... Args>
	void Debug(const Args&... args) {
		std::u8string str = str::Build<std::u8string>(args...);
		host::Debug(std::u8string_view{ str });
	}

	/* build the message and log it using host::fail */
	template <class... Args>
	void Fail [[noreturn]] (const Args&... args) {
		std::u8string str = str::Build<std::u8string>(args...);
		host::Fail(std::u8string_view{ str });
	}
}
