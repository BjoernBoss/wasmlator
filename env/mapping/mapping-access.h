#pragma once

#include "../env-common.h"

namespace env::detail {
	class MappingAccess {
	private:
		env::Process* pProcess = 0;

	public:
		MappingAccess(env::Process* process);

	public:
		uint32_t allocateFromManagement(uint32_t address);
	};
	class MappingBuilder {
	private:
		const env::Process* pProcess = 0;

	public:
		MappingBuilder(const env::Process* process);

	private:
		void fMakeLookup(wasm::Sink& sink, env::CoreState& state, wasm::Table& functions, bool execute) const;

	public:
		void setupCoreImports(wasm::Module& mod, env::CoreState& state) const;
		void setupCoreBody(wasm::Module& mod, env::CoreState& state) const;
		void setupBlockImports(wasm::Module& mod, env::ModuleState& state) const;
	};
}
