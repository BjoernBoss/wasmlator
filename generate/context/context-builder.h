/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025 Bjoern Boss Henrichsen */
#pragma once

#include "../gen-common.h"

namespace gen::detail {
	struct ContextState {
		wasm::Function terminate;
		wasm::Function codeException;
		wasm::Memory memory;
	};

	/*
	*	Core-Imports:
	*		void main.main_terminate(i32 code, i64 address);
	*		void main.main_code_exception(i64 address, i32 id);
	*
	*	Body-Imports:
	*		void ctx.main_terminate(i32 code, i64 address);
	*		void ctx.main_code_exception(i64 address, i32 id);
	*/

	class ContextBuilder {
	public:
		void setupCoreImports();
		void setupBlockImports(const wasm::Memory& memory, detail::ContextState& state) const;
	};
}
