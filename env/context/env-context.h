#pragma once

#include "../env-common.h"

namespace env {
	using id_t = uint32_t;

	class Context {
	private:
		std::u8string pName;
		std::u8string pLogHeader;
		std::u8string pSelfName;
		env::id_t pId{ 0 };

	public:
		Context(std::u8string_view name);
		Context(env::Context&&) = delete;
		Context(const env::Context&) = delete;
		~Context();

	private:
		template <class... Args>
		void fLog(const Args&... args) const {
			util::log(pLogHeader, args...);
		}
		template <class... Args>
		void fDebug(const Args&... args) const {
			util::debug(pLogHeader, args...);
		}
		template <class... Args>
		void fFail(const Args&... args) const {
			util::fail(pLogHeader, args...);
		}

	public:
		bool setCore(const uint8_t* data, size_t size);
		const std::u8string& name() const;
		const std::u8string& selfName() const;
		env::id_t id() const;

	public:
		template <class... Args>
		void log(const Args&... args) const {
			fLog(args...);
		}
		template <class... Args>
		void debug(const Args&... args) const {
			fDebug(args...);
		}
		template <class... Args>
		void fail(const Args&... args) const {
			fFail(args...);
		}
	};
}
