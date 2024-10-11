#include "generate.h"

namespace I = wasm::inst;

void glue::SetupBlocksFunctions(glue::State& state) {
	/* export the flush_blocks function and call the function from the corresponding core module */
	if (true) {
		wasm::Prototype prototype = state.module.prototype(u8"blocks_flush_type",
			{ { u8"id", wasm::Type::i32 } },
			{}
		);
		wasm::Sink sink{ state.module.function(u8"blocks_flush_blocks", prototype, wasm::Export{}) };
		sink[I::Local::Get(sink.parameter(0))];
		sink[I::U32::Const(glue::CoreMapping::_count)];
		sink[I::U32::Mul()];
		sink[I::U32::Const(glue::CoreMapping::flushBlocks)];
		sink[I::U32::Add()];
		sink[I::Call::IndirectTail(state.coreFunctions, {}, {})];
	}

}
