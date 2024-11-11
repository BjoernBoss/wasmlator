#pragma once

#include "../gen-common.h"
#include "../glue/glue-state.h"
#include "../environment/interact/env-interact.h"

namespace gen::detail {
	struct InteractState {
		wasm::Function invokeVoid;
		wasm::Function invokeParam;
	};

	/*
	*	Core-Exports to Main:
	*		void int_invoke_void(i32 index);
	*		i64 int_invoke_param(i32 index, i64 param);
	*
	*	Body-Imports:
	*		void main.main_invoke_void(i32 index);
	*		i64 main.main_invoke_param(i64 param, i32 index);
	*/

	class InteractBuilder {
	public:
		void setupGlueMappings(detail::GlueState& glue);
		void finalizeCoreBody(wasm::Module& mod) const;
		void setupBlockImports(wasm::Module& mod, detail::InteractState& state) const;
	};
}
