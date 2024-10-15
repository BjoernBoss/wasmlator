#include "../env-process.h"

namespace I = wasm::inst;

env::detail::ContextAccess::ContextAccess(env::Process* process) : pProcess{ process } {}
bool env::detail::ContextAccess::create(std::function<void(env::guest_t)> translate, std::function<void(int32_t)> terminated) {
	return pProcess->context().fCreate(translate, terminated);
}

env::detail::ContextBuilder::ContextBuilder(const env::Process* process) : pProcess{ process } {}
void env::detail::ContextBuilder::setupCoreImports(wasm::Module& mod, env::CoreState& state) const {
	/* add the import to the translate-function */
	wasm::Prototype prototype = mod.prototype(u8"ctx_translate_type",
		{ { u8"process", wasm::Type::i64 }, { u8"addr", wasm::Type::i64 } },
		{}
	);
	state.ctx.translate = mod.function(u8"translate", prototype, wasm::Import{ u8"ctx" });

	/* add the import to the terminated-function */
	prototype = mod.prototype(u8"ctx_terminated_type",
		{ { u8"process", wasm::Type::i64 }, { u8"code", wasm::Type::i32 } },
		{}
	);
	state.ctx.terminated = mod.function(u8"terminated", prototype, wasm::Import{ u8"ctx" });
}
void env::detail::ContextBuilder::setupCoreBody(wasm::Module& mod, env::CoreState& state) const {
	/* add the exit function */
	{
		wasm::Prototype prototype = mod.prototype(u8"ctx_exit_type", { { u8"code", wasm::Type::i32 } }, {});
		wasm::Sink sink{ mod.function(u8"ctx_exit", prototype, wasm::Export{}) };

		/* simply perform the termination */
		sink[I::U64::Const(pProcess)];
		sink[I::Local::Get(sink.parameter(0))];
		sink[I::Call::Tail(state.ctx.terminated)];
	}
}
void env::detail::ContextBuilder::setupBlockImports(wasm::Module& mod, env::ModuleState& state) const {
	/* add the import to the exit-function */
	wasm::Prototype prototype = mod.prototype(u8"ctx_exit_type",
		{ { u8"code", wasm::Type::i32 } },
		{}
	);
	state.ctx.exit = mod.function(u8"exit", prototype, wasm::Import{ u8"ctx" });
}
