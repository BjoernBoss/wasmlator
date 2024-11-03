#pragma once

#include "trans-common.h"
#include "memory/memory-writer.h"
#include "context/context-writer.h"

namespace trans {
	class Writer {
	private:
		wasm::Sink pSink;
		detail::MemoryWriter pMemory;
		detail::ContextWriter pContext;

	public:
		Writer(trans::Writer&&) = delete;
		Writer(const trans::Writer&) = delete;

	public:
		/* expects [i64] address on top of stack and writes value to stack */
		void read(uint32_t cacheIndex, trans::MemoryType type) const;

		/* expects [i64] address on top of stack and writes value to stack */
		void code(uint32_t cacheIndex, trans::MemoryType type) const;

		/* expects [i64] address on top of stack and value in variable */
		void write(const wasm::Variable& value, uint32_t cacheIndex, trans::MemoryType type) const;

		/* expects [i32] result-code on top of stack */
		void exit() const;

		/* writes value from context to stack */
		void ctxRead(uint32_t offset, trans::MemoryType type) const;

		/* writes variable to the context */
		void ctxWrite(const wasm::Variable& value, uint32_t offset, trans::MemoryType type) const;
	};
}
