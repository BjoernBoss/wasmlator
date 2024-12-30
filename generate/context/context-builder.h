#pragma once

#include "../gen-common.h"

namespace gen::detail {
	struct ContextState {
		wasm::Function terminate;
		wasm::Function notDecodable;
		wasm::Function notReachable;
		wasm::Memory memory;
	};

	/*
	*	Core-Imports:
	*		void main.main_terminate(i32 code, i64 address);
	*		void main.main_not_decodable(i64 address);
	*		void main.main_not_reachable(i64 address);
	*
	*	Body-Imports:
	*		void ctx.main_terminate(i32 code, i64 address);
	*		void ctx.main_not_decodable(i64 address);
	*		void ctx.main_not_reachable(i64 address);
	*/

	class ContextBuilder {
	public:
		void setupCoreImports();
		void setupBlockImports(const wasm::Memory& memory, detail::ContextState& state) const;
	};
}
