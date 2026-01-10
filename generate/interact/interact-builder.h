/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025-2026 Bjoern Boss Henrichsen */
#pragma once

#include "../gen-common.h"
#include "../glue/glue-state.h"

namespace gen::detail {
	struct InteractState {
		wasm::Function invokeVoid;
		wasm::Function invokeParam;
	};

	/*
	*	Core-Imports:
	*		void main.main_invoke_void(i32 index);
	*		i64 main.main_invoke_param(i64 param, i32 index);
	*
	*	Core-Exports to Main:
	*		void int_call_void(i32 index);
	*		i64 int_call_param(i64 param, i32 index);
	*
	*	Body-Imports:
	*		void int.main_invoke_void(i32 index);
	*		i64 int.main_invoke_param(i64 param, i32 index);
	*/

	class InteractBuilder {
	public:
		void setupGlueMappings(detail::GlueState& glue);
		void setupCoreImports() const;
		void finalizeCoreBody() const;
		void setupBlockImports(detail::InteractState& state) const;
	};
}
