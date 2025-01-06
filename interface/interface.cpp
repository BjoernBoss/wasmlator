#include "interface.h"

#include "../environment/process/process-bridge.h"
#include "../environment/memory/memory-bridge.h"
#include "../environment/context/context-bridge.h"
#include "../environment/mapping/mapping-bridge.h"
#include "../environment/interact/interact-bridge.h"

static host::Logger logger{ u8"interface" };

char8_t* main_allocate_command(uint32_t size) {
	/* allocate a raw buffer of the given size and return it */
	return new char8_t[size];
}
void main_command(char8_t* ptr, uint32_t size) {
	try {
		/* first write the raw buffer to a std::u8string, in order for exceptions
		*	to free the memory properly, and pass it to the handler */
		std::u8string command{ ptr, size };
		delete[] ptr;

		HandleCommand(command);
	}

	/* catch fatal exceptions and ignore them, as they will already have been
	*	logged and otherwise log the occurrance of the unknown exception */
	catch (const host::FatalException&) {}
	catch (...) {
		/* log with level to ensure no fatal-exception is thrown again */
		logger.level(host::LogLevel::fatal, u8"Unhandled exception caught [main_command]");
	}
}

void main_core_loaded(uint32_t process) {
	try {
		env::detail::ProcessBridge::CoreLoaded(process);
	}

	/* catch fatal exceptions and ignore them, as they will already have been
	*	logged and otherwise log the occurrance of the unknown exception */
	catch (const host::FatalException&) {}
	catch (...) {
		/* log with level to ensure no fatal-exception is thrown again */
		logger.level(host::LogLevel::fatal, u8"Unhandled exception caught [main_core_loaded]");
	}
}

void main_block_loaded(uint32_t process) {
	try {
		env::detail::ProcessBridge::BlockLoaded(process);
	}

	/* catch fatal exceptions and ignore them, as they will already have been
	*	logged and otherwise log the occurrance of the unknown exception */
	catch (const host::FatalException&) {}
	catch (...) {
		/* log with level to ensure no fatal-exception is thrown again */
		logger.level(host::LogLevel::fatal, u8"Unhandled exception caught [main_core_loaded]");
	}
}

void main_terminate(int32_t code, uint64_t address) {
	env::detail::ContextBridge::Terminate(code, address);
}
void main_code_exception(uint64_t address, uint32_t id) {
	env::detail::ContextBridge::CodeException(address, id);
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
