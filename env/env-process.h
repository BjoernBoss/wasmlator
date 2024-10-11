#pragma once

#include "env-common.h"
#include "memory/env-memory.h"
#include "context/env-context.h"

namespace env {
	class Process {
	private:
		env::Context pContext;
		env::Memory pMemory;
		uint32_t pManagementPages = 0;

	public:
		Process(std::u8string_view name, uint32_t cacheSize);
		Process(env::Process&&) = delete;
		Process(const env::Process&) = delete;
		~Process() = default;

	public:
		void setupCoreModule(wasm::Module& mod);
		env::BlockState setupBlockModule(wasm::Module& mod) const;

	public:
		const env::Context& context() const;
		env::Context& context();
		const env::Memory& memory() const;
		env::Memory& memory();

	public:
		template <class... Args>
		void log(const Args&... args) const {
			util::log(u8"Process ", pContext.logHeader(), args...);
		}
		template <class... Args>
		void debug(const Args&... args) const {
			util::log(u8"Process ", pContext.logHeader(), args...);
		}
		template <class... Args>
		void fail(const Args&... args) const {
			util::fail(u8"Failure ", pContext.logHeader(), args...);
		}
	};
}
