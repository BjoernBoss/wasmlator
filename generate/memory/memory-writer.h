/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025 Bjoern Boss Henrichsen */
#pragma once

#include "../gen-common.h"
#include "memory-builder.h"

namespace gen::detail {
	class MemoryWriter {
		friend class detail::MemoryBuilder;
	private:
		const detail::MemoryState& pState;
		wasm::Variable pAddress;
		wasm::Variable pOffset;
		wasm::Variable pValuei32;
		wasm::Variable pValuei64;
		wasm::Variable pValuef32;
		wasm::Variable pValuef64;

	public:
		MemoryWriter(const detail::MemoryState& state);

	private:
		void fCheckCache(uint32_t cache) const;

	private:
		void fMakeRead(uint32_t cache, gen::MemoryType type, env::guest_t address, const wasm::Function* code);
		void fMakeWrite(uint32_t cache, gen::MemoryType type, env::guest_t address, env::guest_t nextAddress);

	public:
		void makeRead(uint32_t cacheIndex, gen::MemoryType type, env::guest_t address);
		void makeWrite(uint32_t cacheIndex, gen::MemoryType type, env::guest_t address, env::guest_t nextAddress);
	};
}
