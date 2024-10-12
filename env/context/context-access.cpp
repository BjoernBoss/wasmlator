#include "../env-process.h"

env::detail::ContextInteract::ContextInteract(env::Process* process) : pProcess{ process } {}
bool env::detail::ContextInteract::create(std::function<void(env::guest_t)> translate) {
	return pProcess->context().fCreate(translate);
}
bool env::detail::ContextInteract::setCore(const uint8_t* data, size_t size, std::function<void(bool)> callback) {
	return pProcess->context().fSetCore(data, size, callback);
}

env::detail::ContextBuilder::ContextBuilder(env::Process* process) : pProcess{ process } {}
void env::detail::ContextBuilder::setupCoreImports(wasm::Module& mod, env::CoreState& state) {
	/* add the import to the translate-function */
	wasm::Prototype prototype = mod.prototype(u8"ctx_translate_type",
		{ { u8"process", wasm::Type::i64 }, { u8"addr", wasm::Type::i64 } },
		{}
	);
	state.ctx_core.translate = mod.function(u8"translate", prototype, wasm::Import{ u8"context" });
}
