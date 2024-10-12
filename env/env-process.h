#pragma once

#include "env-common.h"
#include "blocks/env-blocks.h"
#include "memory/env-memory.h"
#include "context/env-context.h"

namespace env {
	class Process {
	private:
		env::Context pContext;
		env::Memory pMemory;
		env::Blocks pBlocks;
		uint32_t pManagementPages = 0;

	public:
		Process(std::u8string_view name, uint32_t cacheSize);
		Process(env::Process&&) = delete;
		Process(const env::Process&) = delete;
		~Process() = default;

	public:
		env::ModuleState setupCoreModule(wasm::Module& mod);
		env::ModuleState setupBlockModule(wasm::Module& mod);

	public:
		const env::Context& context() const;
		env::Context& context();
		const env::Memory& memory() const;
		env::Memory& memory();
		const env::Blocks& blocks() const;
		env::Blocks& blocks();

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
