#pragma once

#include "../gen-common.h"
#include "context-builder.h"

namespace gen::detail {
	class ContextWriter {
	private:
		const detail::ContextState& pState;
		wasm::Sink& pSink;

	public:
		ContextWriter(const detail::ContextState& state, wasm::Sink& sink);

	private:
		void fCheckRange(uint32_t offset, gen::MemoryType type) const;

	public:
		void makeExit() const;
		void makeRead(uint32_t offset, gen::MemoryType type) const;
		void makeWrite(const wasm::Variable& value, uint32_t offset, gen::MemoryType type) const;
	};
}
