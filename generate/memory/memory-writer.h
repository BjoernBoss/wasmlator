#pragma once

#include "../gen-common.h"
#include "memory-builder.h"

namespace gen::detail {
	class MemoryWriter {
		friend class detail::MemoryBuilder;
	private:
		const detail::MemoryState& pState;
		wasm::Sink& pSink;
		mutable wasm::Variable pAccess;
		mutable wasm::Variable pValuei32;
		mutable wasm::Variable pValuei64;
		mutable wasm::Variable pValuef32;
		mutable wasm::Variable pValuef64;

	public:
		MemoryWriter(const detail::MemoryState& state, wasm::Sink& sink);

	private:
		void fCheckCache(uint32_t cache) const;
		void fMakeAddress(uint32_t cache, const wasm::Function& lookup, gen::MemoryType type, env::guest_t address) const;
		void fMakeRead(uint32_t cache, gen::MemoryType type, env::guest_t address) const;
		void fMakeCode(uint32_t cache, gen::MemoryType type, env::guest_t address) const;
		void fMakeWrite(uint32_t cache, gen::MemoryType type, env::guest_t address) const;

	public:
		void makeRead(uint32_t cacheIndex, gen::MemoryType type, env::guest_t address) const;
		void makeCode(uint32_t cacheIndex, gen::MemoryType type, env::guest_t address) const;
		void makeWrite(uint32_t cacheIndex, gen::MemoryType type, env::guest_t address) const;
	};
}
