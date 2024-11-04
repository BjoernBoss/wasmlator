#include "context-builder.h"

namespace I = wasm::inst;

void trans::detail::ContextBuilder::setupCoreImports(wasm::Module& mod, detail::ContextState& state) {
	/* add the import to the exit-code-setter function */
	wasm::Prototype prototype = mod.prototype(u8"ctx_terminate_code_type",
		{ { u8"code", wasm::Type::i32 } },
		{}
	);
	pSetExit = mod.function(u8"terminate_code", prototype, wasm::Import{ u8"ctx" });
}
void trans::detail::ContextBuilder::setupCoreBody(wasm::Module& mod, detail::ContextState& state) const {
	/* add the exit-code-setter function */
	{
		wasm::Prototype prototype = mod.prototype(u8"ctx_set_exit_code_type", { { u8"code", wasm::Type::i32 } }, {});
		state.exit = mod.function(u8"ctx_set_exit_code", prototype, wasm::Export{});
		wasm::Sink sink{ state.exit };

		sink[I::Local::Get(sink.parameter(0))];
		sink[I::Call::Tail(pSetExit)];
	}
}
void trans::detail::ContextBuilder::setupBlockImports(wasm::Module& mod, detail::ContextState& state) const {
	/* add the function-import for the exit-code-setter function */
	wasm::Prototype prototype = mod.prototype(u8"ctx_set_exit_code_type", { { u8"code", wasm::Type::i32 } }, {});
	state.exit = mod.function(u8"set_exit_code", prototype, wasm::Import{ u8"ctx" });
}
