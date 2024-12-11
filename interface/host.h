#pragma once

#include <ustring/ustring.h>

namespace host {
	enum class LogLevel : uint8_t {
		none,
		warn,
		log,
		info,
		debug,
		trace
	};

	/* set the current logging level (none implies only fatal is presented) */
	void SetLogLevel(host::LogLevel level);

	/* fetch the current logging-level */
	host::LogLevel GetLogLevel();

	class Logger {
	private:
		/* fatal is mapped to 'F' */
		static constexpr const char8_t LevelMap[] = { u8'?', u8'W', u8'L', u8'I', u8'D', u8'T' };

	private:
		std::u8string pSelf;

	public:
		Logger(std::u8string_view self);

	private:
		void fLog(std::u8string_view msg) const;
		void fFatal [[noreturn]] (std::u8string_view msg) const;

	private:
		template <class... Args>
		void fBuildLevel(host::LogLevel level, const Args&... args) const {
			/* only build/format the string if it will be printed */
			if (host::GetLogLevel() >= level && level != host::LogLevel::none)
				fLog(str::u8::Build(LevelMap[size_t(level)], u8":[", pSelf, u8"] ", args...));
		}
		template <class... Args>
		void fFormatLevel(host::LogLevel level, std::u8string_view fmt, const Args&... args) const {
			/* only build/format the string if it will be printed */
			if (host::GetLogLevel() >= level && level != host::LogLevel::none)
				fLog(str::u8::Build(LevelMap[size_t(level)], u8":[", pSelf, u8"] ", str::u8::Format(fmt, args...)));
		}

	public:
		/* build the message and write it out as [trace] */
		template <class... Args>
		void trace(const Args&... args) const {
			fBuildLevel(host::LogLevel::trace, args...);
		}

		/* format the message and write it out as [trace] */
		template <class... Args>
		void fmtTrace(std::u8string_view fmt, const Args&... args) const {
			fFormatLevel(host::LogLevel::trace, fmt, args...);
		}

		/* build the message and write it out as [debug] */
		template <class... Args>
		void debug(const Args&... args) const {
			fBuildLevel(host::LogLevel::debug, args...);
		}

		/* format the message and write it out as [debug] */
		template <class... Args>
		void fmtDebug(std::u8string_view fmt, const Args&... args) const {
			fFormatLevel(host::LogLevel::debug, fmt, args...);
		}

		/* build the message and write it out as [log] */
		template <class... Args>
		void log(const Args&... args) const {
			fBuildLevel(host::LogLevel::log, args...);
		}

		/* format the message and write it out as [log] */
		template <class... Args>
		void fmtLog(std::u8string_view fmt, const Args&... args) const {
			fFormatLevel(host::LogLevel::log, fmt, args...);
		}

		/* build the message and write it out as [info] */
		template <class... Args>
		void info(const Args&... args) const {
			fBuildLevel(host::LogLevel::info, args...);
		}

		/* format the message and write it out as [info] */
		template <class... Args>
		void fmtInfo(std::u8string_view fmt, const Args&... args) const {
			fFormatLevel(host::LogLevel::info, fmt, args...);
		}

		/* build the message and write it out as [warning] */
		template <class... Args>
		void warn(const Args&... args) const {
			fBuildLevel(host::LogLevel::warn, args...);
		}

		/* format the message and write it out as [warn] */
		template <class... Args>
		void fmtWarn(std::u8string_view fmt, const Args&... args) const {
			fFormatLevel(host::LogLevel::warn, fmt, args...);
		}

		/* build the message and write it out using the given level */
		template <class... Args>
		void level(host::LogLevel level, const Args&... args) const {
			fBuildLevel(level, args...);
		}

		/* format the message and write it out using the given level */
		template <class... Args>
		void fmtLevel(host::LogLevel level, std::u8string_view fmt, const Args&... args) const {
			fFormatLevel(level, fmt, args...);
		}

		/* build the message and write it out as [fatal] and terminate the execution */
		template <class... Args>
		void fatal [[noreturn]] (const Args&... args) const {
			fFatal(str::u8::Build(u8"F:[", pSelf, u8"] ", args...));
		}

		/* format the message and write it out as [fatal] and terminate the execution */
		template <class... Args>
		void fmtFatal [[noreturn]] (std::u8string_view fmt, const Args&... args) const {
			fFatal(str::u8::Build(u8"F:[", pSelf, u8"] ", str::u8::Format(fmt, args...)));
		}
	};
}
