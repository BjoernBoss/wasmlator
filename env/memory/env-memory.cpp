#include "../env-process.h"
#include "memory-bridge.h"

namespace I = wasm::inst;

env::Memory::Memory(env::Process* process, uint32_t cacheSize) : pMapper{ process, uint32_t(env::PhysPageAligned(env::InitAllocBytes)) }, pInteraction{ process, cacheSize } {}

void env::Memory::setupCoreImports(wasm::Module& mod, env::CoreState& state) {
	pInteraction.setupCoreImports(mod, state);
}
void env::Memory::setupCoreBody(wasm::Module& mod, env::CoreState& state) const {
	pInteraction.setupCoreBody(mod, state);
	pMapper.setupCoreBody(mod, state);
}
void env::Memory::setupBlockImports(wasm::Module& mod, env::BlockState& state) const {
	pInteraction.setupBlockImports(mod, state);
}

void env::Memory::makeRead(const wasm::Variable& i64Address, const env::ModuleState& state, uint32_t cacheIndex, env::MemoryType type) const {
	pInteraction.makeRead(i64Address, state, cacheIndex, type);
}
void env::Memory::makeWrite(const wasm::Variable& i64Address, const wasm::Variable& value, const env::ModuleState& state, uint32_t cacheIndex, env::MemoryType type) const {
	pInteraction.makeWrite(i64Address, value, state, cacheIndex, type);
}
void env::Memory::makeExecute(const wasm::Variable& i64Address, const env::ModuleState& state, uint32_t cacheIndex, env::MemoryType type) const {
	pInteraction.makeExecute(i64Address, state, cacheIndex, type);
}

bool env::Memory::mmap(env::guest_t address, uint32_t size, uint32_t usage) {
	return pMapper.mmap(address, size, usage);
}
void env::Memory::munmap(env::guest_t address, uint32_t size) {
	return pMapper.munmap(address, size);
}
void env::Memory::mprotect(env::guest_t address, uint32_t size, uint32_t usage) {
	return pMapper.mprotect(address, size, usage);
}
