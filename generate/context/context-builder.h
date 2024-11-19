#pragma once

#include "../gen-common.h"
#include "../glue/glue-state.h"

namespace gen::detail {
	struct ContextState {
		wasm::Function terminate;
		wasm::Memory management;
	};
	struct ContextShared {
		wasm::Function notDecodable;
		wasm::Function notReachable;
	};

	/*
	*	Core-Imports:
	*		void main.main_terminate(i32 code);
	*		void main.main_not_decodable(i64 address);
	*		void main.main_unreachable(i64 address);
	*
	*	Core-Exports to Main:
	*		i64 ctx_read(i32 offset, i32 size);
	*		void ctx_write(i32 offset, i32 size, i64 value);
	*
	*	Body-Imports:
	*		void ctx.main_terminate(i32 code);
	*		void ctx.main_not_decodable(i64 address);
	*		void ctx.main_unreachable(i64 address);
	*/

	class ContextBuilder {
	public:
		void setupGlueMappings(detail::GlueState& glue);
		void setupCoreImports(wasm::Module& mod) const;
		void setupCoreBody(wasm::Module& mod, const wasm::Memory& management) const;
		void setupBlockImports(wasm::Module& mod, const wasm::Memory& management, detail::ContextShared& shared, detail::ContextState& state) const;
	};
}
