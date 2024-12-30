#pragma once

#include "../gen-common.h"
#include "context-builder.h"

namespace gen::detail {
	class ContextWriter {
	private:
		const detail::ContextState& pState;
		mutable wasm::Variable pValuei32;
		mutable wasm::Variable pValuei64;
		mutable wasm::Variable pValuef32;
		mutable wasm::Variable pValuef64;

	public:
		ContextWriter(const detail::ContextState& state);

	private:
		void fCheckRange(uint32_t offset, gen::MemoryType type) const;

	public:
		void makeRead(uint32_t offset, gen::MemoryType type) const;
		void makeWrite(uint32_t offset, gen::MemoryType type) const;
		void makeTerminate(env::guest_t address) const;
		void makeNotDecodable(env::guest_t address) const;
		void makeNotReachable(env::guest_t address) const;
	};
}
