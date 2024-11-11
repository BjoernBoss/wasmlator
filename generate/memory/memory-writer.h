#pragma once

#include "../gen-common.h"
#include "memory-builder.h"

namespace gen::detail {
	class MemoryWriter {
		friend class detail::MemoryBuilder;
	private:
		const detail::MemoryState& pState;
		wasm::Sink& pSink;
		mutable wasm::Variable pTempAddress;

	public:
		MemoryWriter(const detail::MemoryState& state, wasm::Sink& sink);

	private:
		void fCheckCache(uint32_t cache) const;
		void fMakeAddress(uint32_t cache, const wasm::Function& lookup, gen::MemoryType type) const;
		void fMakeRead(uint32_t cache, gen::MemoryType type) const;
		void fMakeCode(uint32_t cache, gen::MemoryType type) const;
		void fMakeWrite(const wasm::Variable& value, uint32_t cache, gen::MemoryType type) const;

	public:
		void makeRead(uint32_t cacheIndex, gen::MemoryType type) const;
		void makeCode(uint32_t cacheIndex, gen::MemoryType type) const;
		void makeWrite(const wasm::Variable& value, uint32_t cacheIndex, gen::MemoryType type) const;
	};
}
