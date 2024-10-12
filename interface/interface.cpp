#include "interface.h"

#include "../env/context/context-bridge.h"
#include "../env/memory/memory-bridge.h"
#include "../env/blocks/blocks-bridge.h"

void ctx_core_loaded(uint64_t process, uint32_t succeeded) {
	env::bridge::Context::CoreLoaded(process, succeeded > 0);
}
void ctx_translate(uint64_t process, uint64_t address) {
	env::bridge::Context::Translate(process, address);
}

uint32_t blocks_lookup_complex(uint64_t process, uint64_t address) {
	return env::bridge::Blocks::LookupComplex(process, address);
}
void blocks_flushed(uint64_t process) {
	return env::bridge::Blocks::Flushed(process);
}
void blocks_associate(uint64_t process, uint64_t address, uint32_t index) {
	return env::bridge::Blocks::Associate(process, address, index);
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
