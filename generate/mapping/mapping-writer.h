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
		void makeGetFunction() const;
		void makeDirectInvoke() const;
		void makeTailInvoke() const;
	};
}
