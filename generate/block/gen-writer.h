#pragma once

#include "../gen-common.h"
#include "address-writer.h"
#include "gen-superblock.h"
#include "../memory/memory-writer.h"
#include "../context/context-writer.h"
#include "../interact/interact-writer.h"

namespace gen {
	class Writer;

	/* to fulfill a write-operation (must happen before the writer runs out of scope) */
	class FulFill {
		friend class gen::Writer;
	private:
		enum class Operation : uint8_t {
			none,
			memory,
			context,
			host
		};

	private:
		const gen::Writer* pWriter = 0;
		env::guest_t pAddress = 0;
		uint32_t pCacheIndex = 0;
		gen::MemoryType pType = gen::MemoryType::i64;
		Operation pOperation = Operation::none;

	public:
		FulFill() = default;

	private:
		FulFill(const gen::Writer* writer, env::guest_t address, uint32_t cacheIndex, gen::MemoryType type, Operation operation);

	public:
		/* expects value on top of stack */
		void now();
	};

	class Writer {
		friend class gen::Block;
		friend class gen::FulFill;
	private:
		detail::SuperBlock& pSuperBlock;
		detail::MemoryWriter pMemory;
		detail::ContextWriter pContext;
		detail::AddressWriter pAddress;
		detail::InteractWriter pInteract;

	private:
		Writer(detail::SuperBlock& block, const detail::MemoryState& memory, const detail::ContextState& context, const detail::MappingState& mapping, detail::Addresses& addresses, const detail::InteractState& interact);
		Writer(gen::Writer&&) = delete;
		Writer(const gen::Writer&) = delete;

	public:
		/* check if the given jumpDirect/conditionalDirect instruction target can be locally referenced via a label */
		const wasm::Target* hasTarget(env::guest_t target) const;

		/* no expectations
		*	Note: generated code may contain a tail-call */
		void jump(env::guest_t target) const;

		/* expects guest target-address on top of stack
		*	Note: generated code will contain a tail-call */
		void jump() const;

		/* no expectations
		*	Note: generated code may abort the control-flow */
		void call(env::guest_t target, env::guest_t nextAddress) const;

		/* expects guest target-address on top of stack
		*	Note: generated code may abort the control-flow */
		void call(env::guest_t nextAddress) const;

		/* expects guest return-address on top of stack
		*	Note: generated code will contain a return-statement */
		void ret() const;

		/* expects [i64] address on top of stack and writes value to stack
		*	Note: generated code may abort the control-flow */
		void read(uint32_t cacheIndex, gen::MemoryType type, env::guest_t instAddress) const;

		/* expects [i64] address on top of stack and after the value has been pushed [fulfill::now] to be called
		*	Note: generated code may abort the control-flow (not the now-call)
		*	Note: pushes an i32 to the stack */
		gen::FulFill write(uint32_t cacheIndex, gen::MemoryType type, env::guest_t instAddress) const;

		/* writes value from context to stack */
		void get(uint32_t offset, gen::MemoryType type) const;

		/* expectes nothing but after value has been pushed [fulfill::now] will write it to the context
		*	Note: pushes an i32 to the stack */
		gen::FulFill set(uint32_t offset, gen::MemoryType type) const;

		/* writes value from host to the stack */
		void readHost(const void* host, gen::MemoryType type) const;

		/* expectes nothing but after value has been pushed [fulfill::now] will write it to the host at the given address
		*	Note: pushes an i32 to the stack */
		gen::FulFill writeHost(void* host, gen::MemoryType type) const;

		/* expects [i32] result-code on top of stack
		*	Note: generated code will abort the control-flow */
		void terminate(env::guest_t instAddress) const;

		/* no expectations */
		void invokeVoid(uint32_t index) const;

		/* expects [i64] parameter on top of stack and writes [i64] value to stack */
		void invokeParam(uint32_t index) const;
	};
}
