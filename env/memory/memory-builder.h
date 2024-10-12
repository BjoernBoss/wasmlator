#pragma once

#include "memory-common.h"

namespace env::detail {
	class MemoryBuilder {
	private:
		env::Process* pProcess = 0;

	public:
		MemoryBuilder(env::Process* process);

	public:
		void setupCoreImports(wasm::Module& mod, env::CoreState& state);
		void setupCoreBody(wasm::Module& mod, env::CoreState& state) const;
		void setupBlockImports(wasm::Module& mod, env::ModuleState& state) const;
	};
}
