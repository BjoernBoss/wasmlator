#pragma once

#include "../gen-common.h"
#include "memory-builder.h"

namespace gen::detail {
	class MemoryWriter {
		friend class detail::MemoryBuilder;
	private:
		const detail::MemoryState& pState;
		mutable wasm::Variable pAccess;

	public:
		MemoryWriter(const detail::MemoryState& state);

	private:
		void fCheckCache(uint32_t cache) const;
		uint32_t fMakeIndex(uint32_t cache, gen::MemoryType type) const;
		void fMakeAddress(uint32_t cache, const wasm::Function& lookup, gen::MemoryType type, env::guest_t address) const;
		void fMakeRead(uint32_t cache, gen::MemoryType type, env::guest_t address, const wasm::Function* lookup) const;
		void fMakeStartWrite(uint32_t cache, gen::MemoryType type, env::guest_t address, const wasm::Function* lookup) const;
		void fMakeStopWrite(gen::MemoryType type) const;

	public:
		void makeRead(uint32_t cacheIndex, gen::MemoryType type, env::guest_t address) const;
		void makeStartWrite(uint32_t cacheIndex, gen::MemoryType type, env::guest_t address) const;
		void makeEndWrite(gen::MemoryType type) const;
	};
}
