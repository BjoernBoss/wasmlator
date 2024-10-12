#pragma once

#include "../env-common.h"

namespace env::detail {
	class BlocksBuilder {
	private:
		env::Process* pProcess = 0;

	public:
		BlocksBuilder(env::Process* process);

	private:
		void fMakeLookup(wasm::Sink& sink, env::CoreState& state, wasm::Table& functions, bool execute) const;

	public:
		void setupCoreImports(wasm::Module& mod, env::CoreState& state);
		void setupCoreBody(wasm::Module& mod, env::CoreState& state) const;
		void setupBlockImports(wasm::Module& mod, env::ModuleState& state) const;
	};
}
