#pragma once

#include "trans-common.h"
#include "memory/memory-writer.h"

namespace trans {
	class Writer {
	private:
		wasm::Sink pSink;
		detail::MemoryWriter pMemory;

	public:
		Writer(trans::Writer&&) = delete;
		Writer(const trans::Writer&) = delete;

	public:
		/* expects address on top of stack and writes value to stack */
		void read(uint32_t cacheIndex, trans::MemoryType type) const;

		/* expects address on top of stack and writes value to stack */
		void code(uint32_t cacheIndex, trans::MemoryType type) const;

		/* expects address on top of stack and value in variable */
		void write(const wasm::Variable& value, uint32_t cacheIndex, trans::MemoryType type) const;
	};
}
