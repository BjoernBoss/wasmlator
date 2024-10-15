#pragma once

#include "env-common.h"
#include "mapping/env-mapping.h"
#include "memory/env-memory.h"
#include "context/env-context.h"

namespace env {
	class Process {
	private:
		env::Context pContext;
		env::Memory pMemory;
		env::Mapping pMapping;
		uint32_t pManagementPages = 0;
		uint32_t pPhysicalPages = 0;

	private:
		Process(std::u8string_view name);
		Process(env::Process&&) = delete;
		Process(const env::Process&) = delete;
		~Process() = default;

	public:
		static env::Process* Create(std::u8string_view name, uint32_t caches, std::function<void(env::guest_t)> translate);

	public:
		void release();

	public:
		env::ModuleState setupCoreModule(wasm::Module& mod) const;
		env::ModuleState setupBlockModule(wasm::Module& mod) const;

	public:
		const env::Context& context() const;
		env::Context& context();
		const env::Memory& memory() const;
		env::Memory& memory();
		const env::Mapping& mapping() const;
		env::Mapping& mapping();

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
