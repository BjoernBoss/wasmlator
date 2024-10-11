#include "env-process.h"

env::Process::Process(std::u8string_view name, uint32_t cacheSize) : pContext{ name, this }, pMemory{ this, cacheSize } {}

void env::Process::setupCoreModule(wasm::Module& mod) {
	env::CoreState state;

	/* setup the imports (will also reserve memory in the management-block) */
	pMemory.setupCoreImports(mod, state);
	pManagementPages = env::PhysPageCount(state.endOfManagement);

	/* setup the shared components */
	state.physical = mod.memory(u8"memory_physical", wasm::Limit{ env::PhysPageCount(env::InitAllocBytes) }, wasm::Export{});
	state.management = mod.memory(u8"memory_management", wasm::Limit{ pManagementPages, pManagementPages }, wasm::Export{});

	/* setup the body */
	pMemory.setupCoreBody(mod, state);
}
env::BlockState env::Process::setupBlockModule(wasm::Module& mod) const {
	env::BlockState state;

	/* setup the shared components */
	state.physical = mod.memory(u8"memory_physical", wasm::Limit{ env::PhysPageCount(env::InitAllocBytes) }, pContext.imported());
	state.management = mod.memory(u8"memory_management", wasm::Limit{ pManagementPages, pManagementPages }, pContext.imported());

	/* setup the imports */
	pMemory.setupBlockImports(mod, state);
	return state;
}

const env::Context& env::Process::context() const {
	return pContext;
}
env::Context& env::Process::context() {
	return pContext;
}
const env::Memory& env::Process::memory() const {
	return pMemory;
}
env::Memory& env::Process::memory() {
	return pMemory;
}
