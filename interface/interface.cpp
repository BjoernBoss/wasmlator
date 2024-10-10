#include "interface.h"

#include "../env/context/context-bridge.h"
#include "../env/memory/memory-bridge.h"

void ctx_core_loaded(uint64_t self, uint32_t succeeded) {
	env::bridge::Context::CoreLoaded(self, succeeded > 0);
}

uint32_t mem_mmap(uint64_t self, uint64_t address, uint32_t size, uint32_t usage) {
	return (env::bridge::Memory::MMap(self, address, size, usage) ? 1 : 0);
}
void mem_munmap(uint64_t self, uint64_t address, uint32_t size) {
	env::bridge::Memory::MUnmap(self, address, size);
}
void mem_mprotect(uint64_t self, uint64_t address, uint32_t size, uint32_t usage) {
	env::bridge::Memory::MProtect(self, address, size, usage);
}
uint64_t mem_perform_lookup(uint64_t self, uint64_t address, uint32_t size, uint32_t usage) {
	env::bridge::Memory::Lookup(self, address, size, usage);
	return env::bridge::Memory::LookupAddress(self);
}
uint32_t mem_result_physical(uint64_t self) {
	return env::bridge::Memory::LookupPhysical(self);
}
uint32_t mem_result_size(uint64_t self) {
	return env::bridge::Memory::LookupSize(self);
}
