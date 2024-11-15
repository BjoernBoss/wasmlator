#pragma once

#include "../gen-common.h"
#include "../glue/glue-state.h"

namespace gen::detail {
	struct ContextState {
		wasm::Function exit;
		wasm::Memory management;
	};

	/*
	*	Core-Imports:
	*		void main.main_set_exit_code(i32 code);
	*
	*	Core-Exports to Main:
	*		i64 ctx_read(i32 offset, i32 size);
	*		void ctx_write(i32 offset, i32 size, i64 value);
	*
	*	Body-Imports:
	*		void ctx.main_set_exit_code(i32 code);
	*/

	class ContextBuilder {
	public:
		void setupGlueMappings(detail::GlueState& glue);
		void setupCoreImports(wasm::Module& mod) const;
		void setupCoreBody(wasm::Module& mod, const wasm::Memory& management) const;
		void setupBlockImports(wasm::Module& mod, const wasm::Memory& management, detail::ContextState& state) const;
	};
}
