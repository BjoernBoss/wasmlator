#pragma once

#include "../trans-common.h"
#include "../glue/glue-state.h"
#include "../env/context/env-context.h"

namespace trans::detail {
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
	*	Core-Exports to Body:
	*		void ctx_set_exit_code(i32 code);
	*
	*	Body-Imports:
	*		void core.ctx_set_exit_code(i32 code);
	*/

	class ContextBuilder {
	private:
		wasm::Function pSetExit;

	public:
		void setupGlueMappings(detail::GlueState& glue);
		void setupCoreImports(wasm::Module& mod);
		void setupCoreBody(wasm::Module& mod, const wasm::Memory& management) const;
		void setupBlockImports(wasm::Module& mod, const wasm::Memory& management, detail::ContextState& state) const;
	};
}
