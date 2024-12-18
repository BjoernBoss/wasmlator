#include "context-builder.h"

void gen::detail::ContextBuilder::setupBlockImports(const wasm::Memory& memory, detail::ContextState& state) const {
	state.memory = memory;
}
