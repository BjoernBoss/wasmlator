#pragma once

#include <cinttypes>
#include <wasgen/wasm.h>
#include <ustring/ustring.h>

#include "../env/env-process.h"

namespace trans {
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

	struct TranslationException : public str::BuildException {
		template <class... Args>
		constexpr TranslationException(const Args&... args) : str::BuildException{ args... } {}
	};

	enum class InstType : uint8_t {
		primitive,
		jumpDirect,
		jumpIndirect,
		conditionalDirect,
		call,
		endOfBlock,
		invalid
	};

	struct Instruction {
		uintptr_t data = 0;
		env::guest_t target = 0;
		size_t size = 0;
		trans::InstType type = trans::InstType::primitive;
	};
}
