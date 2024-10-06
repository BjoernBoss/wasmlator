#pragma once

#include "../env-common.h"
#include "../context/env-context.h"

namespace env {
	namespace bridge {
		struct Memory;
	}

	using addr_t = uint64_t;

	using physical_t = uint32_t;

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

	struct MemoryState {
		wasm::Memory memory;
		wasm::Memory caches;
		wasm::Function readFunction;
		wasm::Function writeFunction;
		wasm::Function executeFunction;
		wasm::Function mmapFunction;
		wasm::Function munmapFunction;
		wasm::Function mprotectFunction;
		wasm::Function lookup;
		wasm::Function lookupPhysical;
		wasm::Function lookupSize;
	};

	struct MemoryUsage {
		static constexpr uint32_t Read = 0x01;
		static constexpr uint32_t Write = 0x02;
		static constexpr uint32_t Execute = 0x04;
	};
}
