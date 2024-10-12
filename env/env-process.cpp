#include "env-process.h"

env::Process::Process(std::u8string_view name) : pContext{ name, this }, pMemory{ this }, pBlocks{ this } {}

env::ModuleState env::Process::setupCoreModule(wasm::Module& mod, uint32_t caches) {
	constexpr uint32_t initialPageCount = env::PhysPageCount(env::InitAllocBytes);
	env::CoreState state;

	/* setup the imports (will also reserve memory in the management-block) */
	detail::MemoryBuilder{ this }.setupCoreImports(mod, state, caches, initialPageCount);
	detail::BlocksBuilder{ this }.setupCoreImports(mod, state);
	detail::ContextBuilder{ this }.setupCoreImports(mod, state);
	pManagementPages = env::PhysPageCount(state.endOfManagement);

	/* setup the shared components */
	state.module.physical = mod.memory(u8"memory_physical", wasm::Limit{ initialPageCount }, wasm::Export{});
	state.module.management = mod.memory(u8"memory_management", wasm::Limit{ pManagementPages, pManagementPages }, wasm::Export{});

	/* setup the body */
	detail::MemoryBuilder{ this }.setupCoreBody(mod, state);
	detail::BlocksBuilder{ this }.setupCoreBody(mod, state);
	return state.module;
}
env::ModuleState env::Process::setupBlockModule(wasm::Module& mod) {
	env::ModuleState state;

	/* setup the shared components */
	state.physical = mod.memory(u8"memory_physical", wasm::Limit{ env::PhysPageCount(env::InitAllocBytes) }, pContext.imported());
	state.management = mod.memory(u8"memory_management", wasm::Limit{ pManagementPages, pManagementPages }, pContext.imported());

	/* setup the imports */
	detail::MemoryBuilder{ this }.setupBlockImports(mod, state);
	detail::BlocksBuilder{ this }.setupBlockImports(mod, state);
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
const env::Blocks& env::Process::blocks() const {
	return pBlocks;
}
env::Blocks& env::Process::blocks() {
	return pBlocks;
}
