#pragma once

#include "../trans-common.h"
#include "memory-builder.h"

namespace trans::detail {
	class MemoryWriter {
		friend class detail::MemoryBuilder;
	private:
		env::Process* pProcess = 0;
		detail::MemoryState pState;
		mutable wasm::Variable pTempAddress;
		wasm::Sink& pSink;

	public:
		MemoryWriter(env::Process* process, wasm::Sink& sink, const detail::MemoryState& state);

	private:
		void fCheckCache(uint32_t cache) const;
		void fMakeAddress(uint32_t cache, const wasm::Function& lookup, trans::MemoryType type) const;
		void fMakeRead(uint32_t cache, trans::MemoryType type) const;
		void fMakeCode(uint32_t cache, trans::MemoryType type) const;
		void fMakeWrite(const wasm::Variable& value, uint32_t cache, trans::MemoryType type) const;

	public:
		void makeRead(uint32_t cacheIndex, trans::MemoryType type) const;
		void makeCode(uint32_t cacheIndex, trans::MemoryType type) const;
		void makeWrite(const wasm::Variable& value, uint32_t cacheIndex, trans::MemoryType type) const;
	};
}
