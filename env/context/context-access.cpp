#include "../env-process.h"

env::detail::ContextAccess::ContextAccess(env::Process* process) : pProcess{ process } {}
bool env::detail::ContextAccess::create(std::function<void(env::guest_t)> translate) {
	return pProcess->context().fCreate(translate);
}

env::detail::ContextBuilder::ContextBuilder(const env::Process* process) : pProcess{ process } {}
void env::detail::ContextBuilder::setupCoreImports(wasm::Module& mod, env::CoreState& state) const {
	/* add the import to the translate-function */
	wasm::Prototype prototype = mod.prototype(u8"ctx_translate_type",
		{ { u8"process", wasm::Type::i64 }, { u8"addr", wasm::Type::i64 } },
		{}
	);
	state.ctx.translate = mod.function(u8"translate", prototype, wasm::Import{ u8"ctx" });
}
