/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025-2026 Bjoern Boss Henrichsen */
#include "interface.h"

#include "../environment/process/process-bridge.h"
#include "../environment/memory/memory-bridge.h"
#include "../environment/context/context-bridge.h"
#include "../environment/mapping/mapping-bridge.h"
#include "../environment/interact/interact-bridge.h"

static util::Logger logger{ u8"interface" };

struct Cleanup {
	const void* ptr = 0;
	Cleanup(const void* ptr) : ptr{ ptr } {}
	~Cleanup() { if (ptr != 0) delete[] static_cast<const uint8_t*>(ptr); }
};

void main_handle(const char8_t* ptr, uint32_t size) {
	Cleanup _cleanup{ ptr };

	/* pass the command to the command-handler */
	try {
		HandleCommand(std::u8string_view{ ptr, size });
	}

	/* catch fatal exceptions and ignore them, as they will already have been
	*	logged and otherwise log the occurrance of the unknown exception */
	catch (const util::FatalException&) {}
	catch (...) {
		/* log with level to ensure no fatal-exception is thrown again */
		logger.level(util::LogLevel::fatal, u8"Unhandled exception caught [main_handle]");
	}
}
void main_execute(const char8_t* ptr, uint32_t size) {
	Cleanup _cleanup{ ptr };

	/* pass the command to the command-executor */
	try {
		ExecuteCommand(std::u8string_view{ ptr, size });
	}

	/* catch fatal exceptions and ignore them, as they will already have been
	*	logged and otherwise log the occurrance of the unknown exception */
	catch (const util::FatalException&) {}
	catch (...) {
		/* log with level to ensure no fatal-exception is thrown again */
		logger.level(util::LogLevel::fatal, u8"Unhandled exception caught [main_execute]");
	}
}
void main_cleanup() {
	try {
		CleanupExecute();
	}

	/* catch fatal exceptions and ignore them, as they will already have been
	*	logged and otherwise log the occurrance of the unknown exception */
	catch (const util::FatalException&) {}
	catch (...) {
		/* log with level to ensure no fatal-exception is thrown again */
		logger.level(util::LogLevel::fatal, u8"Unhandled exception caught [main_cleanup]");
	}
}
void main_task_completed(uint32_t process, const char8_t* ptr, uint32_t size) {
	Cleanup _cleanup{ ptr };

	/* pass the task to the current process */
	try {
		env::detail::ProcessBridge::TaskCompleted(process, std::u8string_view{ ptr, size });
	}

	/* catch fatal exceptions and ignore them, as they will already have been
	*	logged and otherwise log the occurrance of the unknown exception */
	catch (const util::FatalException&) {}
	catch (...) {
		/* log with level to ensure no fatal-exception is thrown again */
		logger.level(util::LogLevel::fatal, u8"Unhandled exception caught [main_task_completed]");
	}
}
void* main_allocate(uint32_t size) {
	/* allocate a raw buffer of the given size and return it */
	return new uint8_t[size];
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

void main_check_lookup(uint64_t address, uint64_t access, uint32_t size, uint32_t usage, uint32_t cache) {
	env::detail::MemoryBridge::CheckLookup(address, access, size, usage, cache);
}
void main_fast_lookup(uint64_t access, uint32_t usage, uint32_t cache) {
	env::detail::MemoryBridge::FastLookup(access, usage, cache);
}
void main_check_invalidated(uint64_t address) {
	env::detail::MemoryBridge::CheckXInvalidated(address);
}

void main_invoke_void(uint32_t index) {
	env::detail::InteractBridge::InvokeVoid(index);
}
uint64_t main_invoke_param(uint64_t param, uint32_t index) {
	return env::detail::InteractBridge::InvokeParam(param, index);
}
