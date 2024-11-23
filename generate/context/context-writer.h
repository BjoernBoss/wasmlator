#pragma once

#include "../gen-common.h"
#include "context-builder.h"

namespace gen::detail {
	class ContextWriter {
	private:
		const detail::ContextState& pState;
		wasm::Sink& pSink;
		mutable wasm::Variable pValuei32;
		mutable wasm::Variable pValuei64;
		mutable wasm::Variable pValuef32;
		mutable wasm::Variable pValuef64;

	public:
		ContextWriter(const detail::ContextState& state, wasm::Sink& sink);

	private:
		void fCheckRange(uint32_t offset, gen::MemoryType type) const;

	public:
		void makeTerminate(env::guest_t address) const;
		void makeRead(uint32_t offset, gen::MemoryType type) const;
		void makeWrite(uint32_t offset, gen::MemoryType type) const;
	};
}
