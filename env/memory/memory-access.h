#pragma once

#include "../env-common.h"

namespace env::detail {
	class MemoryAccess {
	private:
		env::Process* pProcess = 0;

	public:
		MemoryAccess(env::Process* process);

	public:
		uint32_t configureAndAllocate(uint32_t address, uint32_t caches, uint32_t initialPageCount);
	};

	class MemoryBuilder {
	private:
		const env::Process* pProcess = 0;

	public:
		MemoryBuilder(const env::Process* process);

	public:
		void setupCoreImports(wasm::Module& mod, env::CoreState& state) const;
		void setupCoreBody(wasm::Module& mod, env::CoreState& state) const;
		void setupBlockImports(wasm::Module& mod, env::ModuleState& state) const;
	};
}
