#include "interface.h"

#include "../env/process/process-bridge.h"
#include "../env/memory/memory-bridge.h"
#include "../env/mapping/mapping-bridge.h"

void proc_core_loaded(uint64_t process, uint32_t succeeded) {
	env::bridge::Process::CoreLoaded(process, succeeded > 0);
}
void proc_block_loaded(uint64_t process, uint32_t succeeded) {
	env::bridge::Process::BlockLoaded(process, succeeded > 0);
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
