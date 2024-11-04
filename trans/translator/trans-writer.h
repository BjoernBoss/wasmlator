#pragma once

#include "../trans-common.h"
#include "trans-superblock.h"
#include "../memory/memory-writer.h"
#include "../context/context-writer.h"
#include "../address/address-writer.h"

namespace trans {
	class Translator;

	class Writer {
		friend class trans::Translator;
	private:
		wasm::Sink& pSink;
		detail::SuperBlock& pSuperBlock;
		detail::MemoryWriter pMemory;
		detail::ContextWriter pContext;
		detail::AddressWriter pAddress;

	private:
		Writer(wasm::Sink& sink, detail::SuperBlock& block, const detail::MemoryState& memory, const detail::ContextState& context, const detail::MappingState& mapping, detail::Addresses& addresses);

	public:
		Writer(trans::Writer&&) = delete;
		Writer(const trans::Writer&) = delete;

	public:
		/* sink of the current function representing the given super-block */
		wasm::Sink& sink() const;

		/* check if the jumpDirect/conditionalDirect instruction at the given address can be locally referenced via a label
		*	Note: Must only be used for the instruction previously decoded for the given address */
		const wasm::Target* hasTarget(env::guest_t address) const;

		/* expects [i64] address on top of stack and writes value to stack */
		void read(uint32_t cacheIndex, trans::MemoryType type) const;

		/* expects [i64] address on top of stack and writes value to stack */
		void code(uint32_t cacheIndex, trans::MemoryType type) const;

		/* expects [i64] address on top of stack and value in variable */
		void write(const wasm::Variable& value, uint32_t cacheIndex, trans::MemoryType type) const;

		/* expects [i32] result-code on top of stack
		*	Note: generated code will contain a return-statement */
		void exit() const;

		/* writes value from context to stack */
		void ctxRead(uint32_t offset, trans::MemoryType type) const;

		/* writes variable to the context */
		void ctxWrite(const wasm::Variable& value, uint32_t offset, trans::MemoryType type) const;

		/* no expectations
		*	Note: generated code may contain a return-statement */
		void call(env::guest_t address, env::guest_t nextAddress) const;

		/* expects guest target-address on top of stack
		*	Note: generated code may contain a return-statement */
		void call(env::guest_t nextAddress) const;

		/* no expectations
		*	Note: generated code will contain a tail-call
		*	Note: generated code may contain a return-statement */
		void jump(env::guest_t address) const;

		/* expects guest target-address on top of stack
		*	Note: generated code will contain a tail-call
		*	Note: generated code may contain a return-statement */
		void jump() const;

		/* expects guest return-address on top of stack
		*	Note: generated code will contain a return-statement */
		void ret() const;
	};
}
