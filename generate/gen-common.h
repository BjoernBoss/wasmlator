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

	/* instruction is an abstract description of an instruction with all the paramter
	*	required for the super-block algorithm to setup the blocks properly */
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

	/* translator interface is used to perform the actual translation of super-blocks automatically */
	class Translator {
	private:
		size_t pMaxDepth = 0;

	protected:
		constexpr Translator(size_t maxTranslationDepth) : pMaxDepth{ maxTranslationDepth } {}

	public:
		virtual ~Translator() = default;

	public:
		/* new super-block translation is being started */
		virtual void started(const gen::Writer& writer) = 0;

		/* super-block translation has been completed */
		virtual void completed(const gen::Writer& writer) = 0;

		/* fetch the next instruction at the given address (address of instruction will automatically be set) */
		virtual gen::Instruction fetch(env::guest_t address) = 0;

		/* produce the wasm-code for the given chunk of fetched instructions (guaranteed to not be jumped into) */
		virtual void produce(const gen::Writer& writer, const gen::Instruction* data, size_t count) = 0;

	public:
		constexpr size_t maxDepth() const {
			return pMaxDepth;
		}
	};
}
