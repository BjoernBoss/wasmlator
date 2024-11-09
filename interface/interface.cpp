#include "interface.h"

#include "../env/process/process-bridge.h"
#include "../env/context/context-bridge.h"
#include "../env/memory/memory-bridge.h"
#include "../env/mapping/mapping-bridge.h"

void main_core_loaded(uint32_t succeeded) {
	env::detail::ProcessBridge::CoreLoaded(succeeded > 0);
}

void main_set_exit_code(int32_t code) {
	env::detail::ContextBridge::SetExitCode(code);
}

uint32_t main_resolve(uint64_t address) {
	return env::detail::MappingBridge::Resolve(address);
}
void main_flushed() {
	return env::detail::MappingBridge::Flushed();
}
void main_block_loaded(uint32_t succeeded) {
	return env::detail::MappingBridge::BlockLoaded(succeeded > 0);
}

void main_lookup(uint64_t address, uint32_t size, uint32_t usage) {
	env::detail::MemoryBridge::Lookup(address, size, usage);
}
uint64_t main_result_address() {
	return env::detail::MemoryBridge::LookupAddress();
}
uint32_t main_result_physical() {
	return env::detail::MemoryBridge::LookupPhysical();
}
uint32_t main_result_size() {
	return env::detail::MemoryBridge::LookupSize();
}
