#include "interface.h"

#include "../environment/process/process-bridge.h"
#include "../environment/context/context-bridge.h"
#include "../environment/memory/memory-bridge.h"
#include "../environment/mapping/mapping-bridge.h"
#include "../environment/interact/interact-bridge.h"

void main_core_loaded(uint32_t process, uint32_t succeeded) {
	env::detail::ProcessBridge::CoreLoaded(process, succeeded > 0);
}
void main_block_loaded(uint32_t process, uint32_t succeeded) {
	env::detail::ProcessBridge::BlockLoaded(process, succeeded > 0);
}

void main_terminate(int32_t code) {
	env::detail::ContextBridge::Terminate(code);
}

uint32_t main_resolve(uint64_t address) {
	return env::detail::MappingBridge::Resolve(address);
}
void main_flushed() {
	return env::detail::MappingBridge::Flushed();
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

void main_invoke_void(uint32_t index) {
	env::detail::InteractBridge::InvokeVoid(index);
}
uint64_t main_invoke_param(uint64_t param, uint32_t index) {
	return env::detail::InteractBridge::InvokeParam(param, index);
}
