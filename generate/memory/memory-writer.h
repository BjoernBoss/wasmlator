#pragma once

#include "../gen-common.h"
#include "memory-builder.h"

namespace gen::detail {
	class MemoryWriter {
		friend class detail::MemoryBuilder;
	private:
		struct Access {
			wasm::Variable address;
			wasm::Variable offset;
		};

	private:
		const detail::MemoryState& pState;
		std::vector<Access> pAccess;
		wasm::Variable pValuei32;
		wasm::Variable pValuei64;
		wasm::Variable pValuef32;
		wasm::Variable pValuef64;
		size_t pReserved = 0;

	public:
		MemoryWriter(const detail::MemoryState& state);

	private:
		void fCheckCache(uint32_t cache) const;
		Access fGetVariables();

	private:
		void fMakeRead(uint32_t cache, gen::MemoryType type, env::guest_t address, const wasm::Function* code);
		void fMakeStartWrite(uint32_t cache, gen::MemoryType type);
		void fMakeStopWrite(uint32_t cache, gen::MemoryType type, env::guest_t address, env::guest_t nextAddress);

	public:
		void makeRead(uint32_t cacheIndex, gen::MemoryType type, env::guest_t address);
		void makeStartWrite(uint32_t cacheIndex, gen::MemoryType type);
		void makeEndWrite(uint32_t cacheIndex, gen::MemoryType type, env::guest_t address, env::guest_t nextAddress);
	};
}
