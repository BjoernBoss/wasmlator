#pragma once

#include <wasgen/wasm.h>
#include <ustring/ustring.h>
#include <cinttypes>
#include <queue>
#include <unordered_map>
#include <vector>
#include <set>

#include "../environment/env-common.h"
#include "../interface/host.h"

namespace gen {
	class Writer;

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

	enum class InstType : uint8_t {
		primitive,
		jumpDirect,
		conditionalDirect,
		endOfBlock,
		invalid
	};

	struct Instruction {
	public:
		uintptr_t data = 0;
		env::guest_t target = 0;
		env::guest_t address = 0;
		size_t size = 0;
		gen::InstType type = gen::InstType::primitive;

	public:
		constexpr Instruction() = default;
		constexpr Instruction(gen::InstType type, size_t size, uintptr_t data) : type{ type }, size{ size }, data{ data } {}
		constexpr Instruction(gen::InstType type, env::guest_t target, size_t size, uintptr_t data) : type{ type }, target{ target }, size{ size }, data{ data } {}
	};
}
