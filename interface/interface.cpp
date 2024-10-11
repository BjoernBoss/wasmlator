#include "interface.h"

#include "../env/context/context-bridge.h"
#include "../env/memory/memory-bridge.h"

void ctx_core_loaded(uint64_t process, uint32_t succeeded) {
	env::bridge::Context::CoreLoaded(process, succeeded > 0);
}

uint64_t mem_perform_lookup(uint64_t process, uint64_t address, uint32_t size, uint32_t usage) {
	env::bridge::Memory::Lookup(process, address, size, usage);
	return env::bridge::Memory::LookupAddress(process);
}
uint32_t mem_result_physical(uint64_t process) {
	return env::bridge::Memory::LookupPhysical(process);
}
uint32_t mem_result_size(uint64_t process) {
	return env::bridge::Memory::LookupSize(process);
}
