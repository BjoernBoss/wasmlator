#include "interface.h"

#include "../env/process/process-bridge.h"
#include "../env/context/context-bridge.h"
#include "../env/memory/memory-bridge.h"
#include "../env/mapping/mapping-bridge.h"

void proc_core_loaded(uint32_t succeeded) {
	env::detail::ProcessBridge::CoreLoaded(succeeded > 0);
}
void proc_block_loaded(uint32_t succeeded) {
	env::detail::ProcessBridge::BlockLoaded(succeeded > 0);
}

void ctx_set_exit_code(int32_t code) {
	env::detail::ContextBridge::SetExitCode(code);
}

uint32_t map_resolve(uint64_t address) {
	return env::detail::MappingBridge::Resolve(address);
}
void map_flushed() {
	return env::detail::MappingBridge::Flushed();
}
void map_associate(uint64_t address, uint32_t index) {
	return env::detail::MappingBridge::Associate(address, index);
}

void mem_lookup(uint64_t address, uint32_t size, uint32_t usage) {
	env::detail::MemoryBridge::Lookup(address, size, usage);
}
uint64_t mem_result_address() {
	return env::detail::MemoryBridge::LookupAddress();
}
uint32_t mem_result_physical() {
	return env::detail::MemoryBridge::LookupPhysical();
}
uint32_t mem_result_size() {
	return env::detail::MemoryBridge::LookupSize();
}
