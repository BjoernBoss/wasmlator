#pragma once

#include <ustring/ustring.h>
#include <wasgen/wasm.h>
#include <cinttypes>
#include <string>
#include <type_traits>
#include <functional>

#include "../util/logging.h"

namespace env {
	static constexpr uint64_t VirtPageSize = 0x1000;
	static constexpr uint32_t VirtPageOffset(uint64_t address) {
		return uint32_t(address & (env::VirtPageSize - 1));
	}
	static constexpr uint64_t VirtPageIndex(uint64_t address) {
		return (address / env::VirtPageSize);
	}
	static constexpr uint32_t VirtPageCount(uint64_t bytes) {
		return uint32_t((bytes + env::VirtPageSize - 1) / env::VirtPageSize);
	}
	static constexpr uint64_t VirtPageAligned(uint64_t bytes) {
		return env::VirtPageSize * ((bytes + env::VirtPageSize - 1) / env::VirtPageSize);
	}

	static constexpr uint64_t PhysPageSize = 0x10000;
	static constexpr uint32_t PhysPageOffset(uint64_t address) {
		return uint32_t(address & (env::PhysPageSize - 1));
	}
	static constexpr uint64_t PhysPageIndex(uint64_t address) {
		return (address / env::PhysPageSize);
	}
	static constexpr uint32_t PhysPageCount(uint64_t bytes) {
		return uint32_t((bytes + env::PhysPageSize - 1) / env::PhysPageSize);
	}
	static constexpr uint64_t PhysPageAligned(uint64_t bytes) {
		return env::PhysPageSize * ((bytes + env::PhysPageSize - 1) / env::PhysPageSize);
	}

	static constexpr uint32_t InitAllocBytes = 64 * env::VirtPageSize;
	static constexpr uint32_t MinGrowthBytes = 32 * env::VirtPageSize;
	static constexpr uint32_t ShiftMemoryFactor = 3;

	class Process;

	using addr_t = uint64_t;
	using physical_t = uint32_t;
	using id_t = uint32_t;

	struct ModuleState {
		wasm::Memory physical;
		wasm::Memory management;
		struct {
			wasm::Function read;
			wasm::Function write;
			wasm::Function execute;
		} mem;
	};

	struct CoreState : public env::ModuleState {
		struct {
			wasm::Function lookup;
			wasm::Function getPhysical;
			wasm::Function getSize;
		} lookup;
		uint32_t endOfManagement = 0;
	};

	struct BlockState : public env::ModuleState {
	};
}
