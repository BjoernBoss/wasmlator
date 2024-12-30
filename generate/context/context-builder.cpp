#include "../generate.h"
#include "../environment/process/process-access.h"

void gen::detail::ContextBuilder::setupCoreImports() {
	/* import the main terminate method and pass it through as binding to the blocks */
	wasm::Prototype prototype = gen::Module->prototype(u8"main_terminate_type", { { u8"code", wasm::Type::i32 }, { u8"address", wasm::Type::i64 } }, {});
	gen::Module->function(u8"main_terminate", prototype, wasm::Transport{ u8"main" });
	env::detail::ProcessAccess::AddCoreBinding(u8"ctx", u8"main_terminate");

	/* import the main not decodable method and pass it through as binding to the blocks */
	prototype = gen::Module->prototype(u8"main_not_decodable_type", { { u8"address", wasm::Type::i64 } }, {});
	gen::Module->function(u8"main_not_decodable", prototype, wasm::Transport{ u8"main" });
	env::detail::ProcessAccess::AddCoreBinding(u8"ctx", u8"main_not_decodable");

	/* import the main not reachable method and pass it through as binding to the blocks */
	prototype = gen::Module->prototype(u8"main_not_reachable_type", { { u8"address", wasm::Type::i64 } }, {});
	gen::Module->function(u8"main_not_reachable", prototype, wasm::Transport{ u8"main" });
	env::detail::ProcessAccess::AddCoreBinding(u8"ctx", u8"main_not_reachable");
}
void gen::detail::ContextBuilder::setupBlockImports(const wasm::Memory& memory, detail::ContextState& state) const {
	state.memory = memory;

	/* add the function-import for the terminate function */
	wasm::Prototype prototype = gen::Module->prototype(u8"main_terminate_type", { { u8"code", wasm::Type::i32 }, { u8"address", wasm::Type::i64 } }, {});
	state.terminate = gen::Module->function(u8"main_terminate", prototype, wasm::Import{ u8"ctx" });

	/* add the function-import for the not decodable function */
	prototype = gen::Module->prototype(u8"main_not_decodable_type", { { u8"address", wasm::Type::i64 } }, {});
	state.notDecodable = gen::Module->function(u8"main_not_decodable", prototype, wasm::Import{ u8"ctx" });

	/* add the function-import for the not reachable function */
	prototype = gen::Module->prototype(u8"main_not_reachable_type", { { u8"address", wasm::Type::i64 } }, {});
	state.notReachable = gen::Module->function(u8"main_not_reachable", prototype, wasm::Import{ u8"ctx" });
}
