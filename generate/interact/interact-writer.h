#pragma once

#include "../gen-common.h"
#include "interact-builder.h"

namespace gen::detail {
	class InteractWriter {
	private:
		const detail::InteractState& pState;
		wasm::Sink& pSink;

	public:
		InteractWriter(const detail::InteractState& state, wasm::Sink& sink);

	public:
		void makeVoid(uint32_t index) const;
		void makeParam(uint32_t index) const;
	};
}
