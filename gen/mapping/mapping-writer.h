#pragma once

#include "../gen-common.h"
#include "mapping-builder.h"

namespace gen::detail {
	class MappingWriter {
	private:
		const detail::MappingState& pState;
		wasm::Sink& pSink;

	public:
		MappingWriter(const detail::MappingState& state, wasm::Sink& sink);

	public:
		void makeCheckFailed() const;
		void makeLookup() const;
		void makeLoadFunction() const;
		void makeInvoke() const;
		void makeTailInvoke() const;
	};
}
