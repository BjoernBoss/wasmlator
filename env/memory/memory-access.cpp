#include "../env-process.h"

namespace I = wasm::inst;

env::detail::MemoryAccess::MemoryAccess(env::Process* process) : pProcess{ process } {}
uint32_t env::detail::MemoryAccess::configureAndAllocate(uint32_t address, uint32_t caches, uint32_t initialPageCount) {
	pProcess->memory().pMapper.configure(initialPageCount);
	return pProcess->memory().pInteraction.configureAndAllocate(address, caches);
}

env::detail::MemoryBuilder::MemoryBuilder(const env::Process* process) : pProcess{ process } {}
void env::detail::MemoryBuilder::setupCoreImports(wasm::Module& mod, env::CoreState& state) const {
	pProcess->memory().pInteraction.setupCoreImports(mod, state);
}
void env::detail::MemoryBuilder::setupCoreBody(wasm::Module& mod, env::CoreState& state) const {
	pProcess->memory().pMapper.setupCoreBody(mod, state);
	pProcess->memory().pInteraction.setupCoreBody(mod, state);
}
void env::detail::MemoryBuilder::setupBlockImports(wasm::Module& mod, env::ModuleState& state) const {
	pProcess->memory().pInteraction.setupBlockImports(mod, state);
}
