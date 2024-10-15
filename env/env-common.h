#pragma once

#include <ustring/ustring.h>
#include <wasgen/wasm.h>
#include <cinttypes>
#include <string>
#include <type_traits>
#include <functional>
#include <unordered_map>

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

	static constexpr uint32_t MinBlockList = 4;
	static constexpr uint32_t BlockListGrowth = 2;
	static constexpr uint32_t MinFunctionList = 32;
	static constexpr uint32_t FunctionListGrowth = 16;
	static constexpr uint32_t InitAllocBytes = 64 * env::VirtPageSize;
	static constexpr uint32_t MinGrowthBytes = 32 * env::VirtPageSize;
	static constexpr uint32_t ShiftMemoryFactor = 3;
	static constexpr uint32_t BlockLookupCacheBits = 10;

	using guest_t = uint64_t;
	using physical_t = uint32_t;
	using id_t = uint32_t;

	class Process;

	/* state setup and shared for creating a block/core module */
	struct ModuleState {
		wasm::Memory physical;
		wasm::Memory management;
		struct {
			wasm::Function read;
			wasm::Function write;
			wasm::Function execute;
		} mem;
		struct {
			wasm::Function execute;
			wasm::Function lookup;
		} mapping;
	};

	/* state setup and shared for creating a core module */
	struct CoreState {
		env::ModuleState module;
		struct {
			wasm::Function translate;
		} ctx;
		struct {
			wasm::Function lookup;
			wasm::Function getAddress;
			wasm::Function getPhysical;
			wasm::Function getSize;
		} mem;
		struct {
			wasm::Function resolve;
			wasm::Function flushed;
			wasm::Function associate;
		} mapping;
	};

	/* memory reading type */
	enum class MemoryType : uint8_t {
		u8To32,
		u16To32,
		u8To64,
		u16To64,
		u32To64,
		i8To32,
		i16To32,
		i8To64,
		i16To64,
		i32To64,
		i32,
		i64,
		f32,
		f64
	};

	/* memory page usage flags */
	struct MemoryUsage {
		static constexpr uint32_t Read = 0x01;
		static constexpr uint32_t Write = 0x02;
		static constexpr uint32_t Execute = 0x04;
	};

	/* block exported function entry */
	struct BlockExport {
		std::u8string name;
		env::guest_t address = 0;
	};
}
