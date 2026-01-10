/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025-2026 Bjoern Boss Henrichsen */
#pragma once

#include <wasgen/wasm.h>
#include <ustring/ustring.h>
#include <cinttypes>
#include <queue>
#include <unordered_map>
#include <vector>
#include <set>
#include <memory>
#include <iterator>

#include "../environment/env-common.h"
#include "../util/util-logger.h"

namespace I = wasm::inst;

namespace gen {
	class Block;
	class Writer;
	class Generator;

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
		f64,
		_end
	};

	enum class InstType : uint8_t {
		invalid,
		primitive,
		jumpDirect,
		conditionalDirect,
		endOfBlock
	};

	/* instruction is an abstract description of an instruction with all the paramter
	*	required for the super-block algorithm to setup the blocks properly */
	struct Instruction {
	public:
		uintptr_t self = 0;
		env::guest_t target = 0;
		gen::InstType type = gen::InstType::invalid;
		uint8_t size = 0;

	public:
		constexpr Instruction() = default;
		constexpr Instruction(gen::InstType type, uint8_t size, uintptr_t self) : type{ type }, size{ size }, self{ self } {}
		constexpr Instruction(gen::InstType type, env::guest_t target, uint8_t size, uintptr_t self) : type{ type }, target{ target }, size{ size }, self{ self } {}
	};

	/* translator interface is used to perform the actual translation of super-blocks automatically */
	class Translator {
	protected:
		Translator() = default;

	public:
		virtual ~Translator() = default;

	public:
		/* new super-block translation is being started */
		virtual void started(env::guest_t address) = 0;

		/* super-block translation has been completed */
		virtual void completed() = 0;

		/* fetch the next instruction at the given address */
		virtual gen::Instruction fetch(env::guest_t address) = 0;

		/* produce the wasm-code for the given chunk of fetched instructions (guaranteed to not be jumped into) */
		virtual void produce(env::guest_t address, const uintptr_t* self, size_t count) = 0;
	};
}
