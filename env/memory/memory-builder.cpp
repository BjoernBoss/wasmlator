#include "../env-process.h"

namespace I = wasm::inst;

env::detail::MemoryBuilder::MemoryBuilder(env::Process* process) : pProcess{ process } {}

void env::detail::MemoryBuilder::setupCoreImports(wasm::Module& mod, env::CoreState& state, uint32_t caches, uint32_t initialPageCount) {
	pProcess->memory().pInteraction.setupCoreImports(mod, state, caches);
	pProcess->memory().pMapper.setupCoreImports(initialPageCount);
}
void env::detail::MemoryBuilder::setupCoreBody(wasm::Module& mod, env::CoreState& state) const {
	pProcess->memory().pInteraction.setupCoreBody(mod, state);
	pProcess->memory().pMapper.setupCoreBody(mod, state);
}
void env::detail::MemoryBuilder::setupBlockImports(wasm::Module& mod, env::ModuleState& state) const {
	pProcess->memory().pInteraction.setupBlockImports(mod, state);
}
