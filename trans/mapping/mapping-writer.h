#pragma once

#include "../trans-common.h"
#include "mapping-builder.h"

namespace trans::detail {
	class MappingWriter {
	private:
		detail::MappingState pState;
		wasm::Sink& pSink;

	public:
		MappingWriter(const detail::MappingState& state, wasm::Sink& sink);

	public:
		void makeLookup() const;
		void makeLoadFunction() const;
		void makeInvoke() const;
	};
}
