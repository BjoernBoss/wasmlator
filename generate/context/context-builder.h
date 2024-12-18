#pragma once

#include "../gen-common.h"

namespace gen::detail {
	struct ContextState {
		wasm::Memory memory;
	};

	class ContextBuilder {
	public:
		void setupBlockImports(const wasm::Memory& memory, detail::ContextState& state) const;
	};
}
