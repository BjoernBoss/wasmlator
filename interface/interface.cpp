#include "interface.h"

#include "../environment/process/process-bridge.h"
#include "../environment/memory/memory-bridge.h"
#include "../environment/mapping/mapping-bridge.h"
#include "../environment/interact/interact-bridge.h"

static host::Logger logger{ u8"interface" };

void main_initialize() {
	try {
		StartupProcess();
	}
	catch (...) {
		logger.fatal(u8"Unhandled exception caught [main_initialize]");
	}
}

void main_core_loaded(uint32_t process, uint32_t succeeded) {
	try {
		env::detail::ProcessBridge::CoreLoaded(process, succeeded > 0);
	}
	catch (...) {
		logger.fatal(u8"Unhandled exception caught [main_core_loaded]");
	}
}
void main_block_loaded(uint32_t process, uint32_t succeeded) {
	try {
		env::detail::ProcessBridge::BlockLoaded(process, succeeded > 0);
	}
	catch (...) {
		logger.fatal(u8"Unhandled exception caught [main_core_loaded]");
	}
}

void main_terminate(int32_t code, uint64_t address) {
	env::detail::ProcessBridge::Terminate(code, address);
}
void main_not_decodable(uint64_t address) {
	env::detail::ProcessBridge::NotDecodable(address);
}
void main_not_reachable(uint64_t address) {
	env::detail::ProcessBridge::NotReachable(address);
}
void main_single_step(uint64_t address) {
	env::detail::ProcessBridge::SingleStep(address);
}

uint32_t main_resolve(uint64_t address) {
	return env::detail::MappingBridge::Resolve(address);
}

void main_lookup(uint64_t address, uint64_t access, uint32_t size, uint32_t usage, uint32_t cache) {
	env::detail::MemoryBridge::Lookup(address, access, size, usage, cache);
}

void main_invoke_void(uint32_t index) {
	env::detail::InteractBridge::InvokeVoid(index);
}
uint64_t main_invoke_param(uint64_t param, uint32_t index) {
	return env::detail::InteractBridge::InvokeParam(param, index);
}
