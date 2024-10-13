#include "env-process.h"

env::Process::Process(std::u8string_view name) : pContext{ name, this }, pMemory{ this }, pBlocks{ this } {}

env::Process* env::Process::Create(std::u8string_view name, uint32_t caches, std::function<void(env::guest_t)> translate) {
	env::Process* out = new env::Process{ name };

	/* try to setup the context */
	if (!detail::ContextAccess{ out }.create(translate)) {
		delete out;
		return 0;
	}

	/* initialize the components */
	out->pPhysicalPages = env::PhysPageCount(env::InitAllocBytes);
	uint32_t endOfManagement = 0;
	endOfManagement += detail::BlocksAccess{ out }.allocateFromManagement(endOfManagement);
	endOfManagement += detail::MemoryAccess{ out }.configureAndAllocate(endOfManagement, caches, out->pPhysicalPages);
	out->pManagementPages = env::PhysPageCount(endOfManagement);
	return out;
}

void env::Process::release() {
	delete this;
}

env::ModuleState env::Process::setupCoreModule(wasm::Module& mod) const {
	env::CoreState state;

	/* setup the imports (will also reserve memory in the management-block) */
	detail::MemoryBuilder{ this }.setupCoreImports(mod, state);
	detail::BlocksBuilder{ this }.setupCoreImports(mod, state);
	detail::ContextBuilder{ this }.setupCoreImports(mod, state);

	/* setup the shared components */
	state.module.physical = mod.memory(u8"memory_physical", wasm::Limit{ pPhysicalPages }, wasm::Export{});
	state.module.management = mod.memory(u8"memory_management", wasm::Limit{ pManagementPages, pManagementPages }, wasm::Export{});

	/* setup the body */
	detail::MemoryBuilder{ this }.setupCoreBody(mod, state);
	detail::BlocksBuilder{ this }.setupCoreBody(mod, state);
	return state.module;
}
env::ModuleState env::Process::setupBlockModule(wasm::Module& mod) const {
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
