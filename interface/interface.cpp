#include "interface.h"

#include "../env/context/context-bridge.h"
#include "../env/memory/memory-bridge.h"
#include "../env/mapping/mapping-bridge.h"

void ctx_core_loaded(uint64_t process, uint32_t succeeded) {
	env::bridge::Context::CoreLoaded(process, succeeded > 0);
}
void ctx_block_loaded(uint64_t process, uint32_t succeeded) {
	env::bridge::Context::BlockLoaded(process, succeeded > 0);
}
void ctx_translate(uint64_t process, uint64_t address) {
	env::bridge::Context::Translate(process, address);
}
void ctx_terminated(uint64_t process, int32_t code) {
	env::bridge::Context::Terminated(process, code);
}

uint32_t map_resolve(uint64_t process, uint64_t address) {
	return env::bridge::Mapping::Resolve(process, address);
}
void map_flushed(uint64_t process) {
	return env::bridge::Mapping::Flushed(process);
}
void map_associate(uint64_t process, uint64_t address, uint32_t index) {
	return env::bridge::Mapping::Associate(process, address, index);
}

void mem_lookup(uint64_t process, uint64_t address, uint32_t size, uint32_t usage) {
	env::bridge::Memory::Lookup(process, address, size, usage);
}
uint64_t mem_result_address(uint64_t process) {
	return env::bridge::Memory::LookupAddress(process);
}
uint32_t mem_result_physical(uint64_t process) {
	return env::bridge::Memory::LookupPhysical(process);
}
uint32_t mem_result_size(uint64_t process) {
	return env::bridge::Memory::LookupSize(process);
}
