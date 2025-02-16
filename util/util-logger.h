#pragma once

#include <ustring/ustring.h>

namespace util {
	enum class LogLevel : uint8_t {
		fatal,
		error,
		warn,
		log,
		info,
		debug,
		trace
	};

	/* thrown whenever a fatal message is being logged */
	struct FatalException {
		constexpr FatalException() = default;
	};

	/* configure the logging behavior (fatal will always be passed through) */
	void ConfigureLogging(bool enabled);
	bool LoggingEnabled();

	class Logger;

	namespace detail {
		struct LoggerAccess {
			static util::Logger MakeNullLogger();
		};
	}

	class Logger {
		friend struct detail::LoggerAccess;
	private:
		static constexpr const char8_t LevelMap[] = { u8'F', u8'E', u8'W', u8'L', u8'I', u8'D', u8'T' };

	private:
		std::u8string pFormat;

	private:
		Logger();

	public:
		Logger(std::u8string_view self);

	private:
		void fLog(std::u8string_view msg, bool fatal) const;

	private:
		template <class... Args>
		void fBuildLevel(util::LogLevel level, const Args&... args) const {
			bool fatal = (level == util::LogLevel::fatal);

			/* only build/format the string if it will be printed */
			if (util::LoggingEnabled())
				fLog(str::u8::Build(LevelMap[size_t(level)], u8':', pFormat, args...), fatal);
		}
		template <class... Args>
		void fFormatLevel(util::LogLevel level, std::u8string_view fmt, const Args&... args) const {
			bool fatal = (level == util::LogLevel::fatal);

			/* only build/format the string if it will be printed */
			if (fatal || util::LoggingEnabled())
				fLog(str::u8::Build(LevelMap[size_t(level)], u8':', pFormat, str::u8::Format(fmt, args...)), fatal);
		}

	public:
		/* build the message and write it out as [trace] */
		template <class... Args>
		void trace(const Args&... args) const {
			fBuildLevel(util::LogLevel::trace, args...);
		}

		/* format the message and write it out as [trace] */
		template <class... Args>
		void fmtTrace(std::u8string_view fmt, const Args&... args) const {
			fFormatLevel(util::LogLevel::trace, fmt, args...);
		}

		/* build the message and write it out as [debug] */
		template <class... Args>
		void debug(const Args&... args) const {
			fBuildLevel(util::LogLevel::debug, args...);
		}

		/* format the message and write it out as [debug] */
		template <class... Args>
		void fmtDebug(std::u8string_view fmt, const Args&... args) const {
			fFormatLevel(util::LogLevel::debug, fmt, args...);
		}

		/* build the message and write it out as [log] */
		template <class... Args>
		void log(const Args&... args) const {
			fBuildLevel(util::LogLevel::log, args...);
		}

		/* format the message and write it out as [log] */
		template <class... Args>
		void fmtLog(std::u8string_view fmt, const Args&... args) const {
			fFormatLevel(util::LogLevel::log, fmt, args...);
		}

		/* build the message and write it out as [info] */
		template <class... Args>
		void info(const Args&... args) const {
			fBuildLevel(util::LogLevel::info, args...);
		}

		/* format the message and write it out as [info] */
		template <class... Args>
		void fmtInfo(std::u8string_view fmt, const Args&... args) const {
			fFormatLevel(util::LogLevel::info, fmt, args...);
		}

		/* build the message and write it out as [warn] */
		template <class... Args>
		void warn(const Args&... args) const {
			fBuildLevel(util::LogLevel::warn, args...);
		}

		/* format the message and write it out as [warn] */
		template <class... Args>
		void fmtWarn(std::u8string_view fmt, const Args&... args) const {
			fFormatLevel(util::LogLevel::warn, fmt, args...);
		}

		/* build the message and write it out as [error] */
		template <class... Args>
		void error(const Args&... args) const {
			fBuildLevel(util::LogLevel::error, args...);
		}

		/* format the message and write it out as [error] */
		template <class... Args>
		void fmtError(std::u8string_view fmt, const Args&... args) const {
			fFormatLevel(util::LogLevel::error, fmt, args...);
		}

		/* build the message and write it out as [fatal] and throw a fatal exception */
		template <class... Args>
		void fatal(const Args&... args) const {
			fBuildLevel(util::LogLevel::fatal, args...);
			throw util::FatalException{};
		}

		/* format the message and write it out as [fatal] and throw a fatal exception */
		template <class... Args>
		void fmtFatal(std::u8string_view fmt, const Args&... args) const {
			fFormatLevel(util::LogLevel::fatal, fmt, args...);
			throw util::FatalException{};
		}

		/* build the message and write it out using the given level but will not throw an exception for level=fatal */
		template <class... Args>
		void level(util::LogLevel level, const Args&... args) const {
			fBuildLevel(level, args...);
		}

		/* format the message and write it out using the given level but will not throw an exception for level=fatal */
		template <class... Args>
		void fmtLevel(util::LogLevel level, std::u8string_view fmt, const Args&... args) const {
			fFormatLevel(level, fmt, args...);
		}
	};

	extern util::Logger nullLogger;
}
