#pragma once

#include "../trans-common.h"
#include "../../env/context/env-context.h"

namespace trans::detail {
	struct ContextState {
		wasm::Function exit;
		wasm::Memory management;
	};

	class ContextBuilder {
	private:
		wasm::Function pSetExit;

	public:
		void setupCoreImports(wasm::Module& mod, detail::ContextState& state);
		void setupCoreBody(wasm::Module& mod, detail::ContextState& state) const;
		void setupBlockImports(wasm::Module& mod, detail::ContextState& state) const;
	};
}
