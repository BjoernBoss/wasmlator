/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025-2026 Bjoern Boss Henrichsen */
#include "../generate.h"
#include "../../environment/process/process-access.h"

void gen::detail::ContextBuilder::setupCoreImports() {
	/* import the main terminate method and pass it through as binding to the blocks */
	wasm::Prototype prototype = gen::Module->prototype(u8"main_terminate_type", { { u8"code", wasm::Type::i32 }, { u8"address", wasm::Type::i64 } }, {});
	gen::Module->function(u8"main_terminate", prototype, wasm::Transport{ u8"main" });
	env::detail::ProcessAccess::AddCoreBinding(u8"ctx", u8"main_terminate");

	/* import the main code-exception method and pass it through as binding to the blocks */
	prototype = gen::Module->prototype(u8"main_code_exception_type", { { u8"address", wasm::Type::i64 }, { u8"id", wasm::Type::i32 } }, {});
	gen::Module->function(u8"main_code_exception", prototype, wasm::Transport{ u8"main" });
	env::detail::ProcessAccess::AddCoreBinding(u8"ctx", u8"main_code_exception");
}
void gen::detail::ContextBuilder::setupBlockImports(const wasm::Memory& memory, detail::ContextState& state) const {
	state.memory = memory;

	/* add the function-import for the terminate function */
	wasm::Prototype prototype = gen::Module->prototype(u8"main_terminate_type", { { u8"code", wasm::Type::i32 }, { u8"address", wasm::Type::i64 } }, {});
	state.terminate = gen::Module->function(u8"main_terminate", prototype, wasm::Import{ u8"ctx" });

	/* add the function-import for the code-exception function */
	prototype = gen::Module->prototype(u8"main_code_exception_type", { { u8"address", wasm::Type::i64 }, { u8"id", wasm::Type::i32 } }, {});
	state.codeException = gen::Module->function(u8"main_code_exception", prototype, wasm::Import{ u8"ctx" });
}
