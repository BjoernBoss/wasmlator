#pragma once

#include "../gen-common.h"
#include "memory-builder.h"

namespace gen::detail {
	class MemoryWriter {
		friend class detail::MemoryBuilder;
	private:
		const detail::MemoryState& pState;
		mutable wasm::Variable pAccess;
		mutable wasm::Variable pValuei32;
		mutable wasm::Variable pValuei64;
		mutable wasm::Variable pValuef32;
		mutable wasm::Variable pValuef64;

	public:
		MemoryWriter(const detail::MemoryState& state);

	private:
		void fCheckCache(uint32_t cache) const;
		uint32_t fMakeIndex(uint32_t cache, gen::MemoryType type) const;
		void fMakeRead(uint32_t cache, gen::MemoryType type, env::guest_t address, const wasm::Function* lookup) const;
		void fMakeStartWrite(uint32_t cache, gen::MemoryType type, env::guest_t address, const wasm::Function* lookup) const;
		void fMakeStopWrite(uint32_t cache, gen::MemoryType type, env::guest_t address, const wasm::Function* lookup) const;

	public:
		void makeRead(uint32_t cacheIndex, gen::MemoryType type, env::guest_t address) const;
		void makeStartWrite(uint32_t cacheIndex, gen::MemoryType type, env::guest_t address) const;
		void makeEndWrite(uint32_t cacheIndex, gen::MemoryType type, env::guest_t address) const;
	};
}
