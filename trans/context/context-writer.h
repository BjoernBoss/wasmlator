#pragma once

#include "../trans-common.h"
#include "context-builder.h"

namespace trans::detail {
	class ContextWriter {
	private:
		detail::ContextState pState;
		wasm::Sink& pSink;

	public:
		ContextWriter(const detail::ContextState& state, wasm::Sink& sink);

	private:
		void fCheckRange(uint32_t offset, trans::MemoryType type) const;

	public:
		void makeExit() const;
		void makeRead(uint32_t offset, trans::MemoryType type) const;
		void makeWrite(const wasm::Variable& value, uint32_t offset, trans::MemoryType type) const;
	};
}
