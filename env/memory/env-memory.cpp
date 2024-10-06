#include "env-memory.h"
#include "memory-bridge.h"

namespace I = wasm::inst;

env::Memory::Memory(env::Context& context, uint32_t cacheSize) : pMapper{ context, uint32_t(env::PhysPageAligned(env::InitAllocBytes)) }, pInteraction{ context, pMapper, cacheSize }, pContext{ &context } {}

env::MemoryState env::Memory::setupCoreModule(wasm::Module& mod) const {
	env::MemoryState state;

	/* setup the imports of the components */
	pInteraction.addCoreImports(state, mod);
	pMapper.addCoreImports(state, mod);

	/* add the body of the components */
	pInteraction.addCoreBody(state, mod);
	pMapper.addCoreBody(state, mod);

	return state;
}
env::MemoryState env::Memory::setupBlockModule(wasm::Module& mod) const {
	env::MemoryState state;

	/* setup the imports of the components */
	pInteraction.addBlockImports(state, mod);
	pMapper.addBlockImports(state, mod);
	return state;
}

void env::Memory::makeRead(const wasm::Variable& i64Address, const env::MemoryState& state, uint32_t cacheIndex, env::MemoryType type) const {
	pInteraction.makeRead(i64Address, state, cacheIndex, type);
}
void env::Memory::makeWrite(const wasm::Variable& i64Address, const wasm::Variable& value, const env::MemoryState& state, uint32_t cacheIndex, env::MemoryType type) const {
	pInteraction.makeWrite(i64Address, value, state, cacheIndex, type);
}
void env::Memory::makeExecute(const wasm::Variable& i64Address, const env::MemoryState& state, uint32_t cacheIndex, env::MemoryType type) const {
	pInteraction.makeExecute(i64Address, state, cacheIndex, type);
}

bool env::Memory::mmap(env::addr_t address, uint32_t size, uint32_t usage) {
	return pMapper.mmap(address, size, usage);
}
void env::Memory::munmap(env::addr_t address, uint32_t size) {
	return pMapper.munmap(address, size);
}
void env::Memory::mprotect(env::addr_t address, uint32_t size, uint32_t usage) {
	return pMapper.mprotect(address, size, usage);
}
