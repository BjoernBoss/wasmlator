#pragma once

#include "env-memory.h"

namespace env {
	class Process {
	private:
		env::Memory pMemory;
		std::u8string pName;
		std::u8string pLogHeader;

	private:
		Process() = default;

	public:
		static env::Process Create(std::u8string_view name);

	private:
		template <class... Args>
		void fLog(const Args&... args) const {
			util::log(pLogHeader, args...);
		}
		template <class... Args>
		void fFail(const Args&... args) const {
			util::fail(pLogHeader, args...);
		}

	private:
		std::u8string fCreateMemoryModule();

	public:
		env::Memory& memory();
		const env::Memory& memory() const;

	public:
		template <class... Args>
		void log(const Args&... args) const {
			fLog(args...);
		}
		template <class... Args>
		void fail(const Args&... args) const {
			fFail(args...);
		}

		template <class Type>
		Type read(env::addr_t addr) const {

		}
		template <class Type>
		void write(env::addr_t addr, Type val) {

		}
		template <class Type>
		Type execute(env::addr_t addr) const {

		}
	};
}
