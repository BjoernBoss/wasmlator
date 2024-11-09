#include "context-builder.h"

namespace I = wasm::inst;

void trans::detail::ContextBuilder::setupGlueMappings(detail::GlueState& glue) {
	glue.define(u8"ctx_read", { { u8"offset", wasm::Type::i32 }, { u8"size", wasm::Type::i32 } }, { wasm::Type::i64 });
	glue.define(u8"ctx_write", { { u8"offset", wasm::Type::i32 }, { u8"size", wasm::Type::i32 }, { u8"value", wasm::Type::i64 } }, {});
}
void trans::detail::ContextBuilder::setupCoreImports(wasm::Module& mod) {
	/* add the import to the exit-code-setter function */
	wasm::Prototype prototype = mod.prototype(u8"main_set_exit_code_type", { { u8"code", wasm::Type::i32 } }, {});
	pSetExit = mod.function(u8"main_set_exit_code", prototype, wasm::Import{ u8"main" });
}
void trans::detail::ContextBuilder::setupCoreBody(wasm::Module& mod, const wasm::Memory& management) const {
	/* add the context-read function */
	{
		wasm::Prototype prototype = mod.prototype(u8"ctx_read_type", { { u8"offset", wasm::Type::i32 }, { u8"size", wasm::Type::i32 } }, { wasm::Type::i64 });
		wasm::Sink sink{ mod.function(u8"ctx_read", prototype, wasm::Export{}) };

		sink[I::Local::Get(sink.parameter(0))];
		wasm::Block _block1{ sink, u8"size_1", { wasm::Type::i32 }, { wasm::Type::i32 } };
		wasm::Block _block2{ sink, u8"size_2", { wasm::Type::i32 }, { wasm::Type::i32 } };
		wasm::Block _block4{ sink, u8"size_4", { wasm::Type::i32 }, { wasm::Type::i32 } };
		wasm::Block _block8{ sink, u8"size_8", { wasm::Type::i32 }, { wasm::Type::i32 } };
		sink[I::Local::Get(sink.parameter(1))];
		sink[I::U32::TrailingNulls()];
		sink[I::Branch::Table({ _block1, _block2, _block4 }, _block8)];
		_block1.close();
		sink[I::U64::Load8(management, env::detail::ContextAccess{}.contextAddress())];
		sink[I::Return()];

		_block2.close();
		sink[I::U64::Load16(management, env::detail::ContextAccess{}.contextAddress())];
		sink[I::Return()];

		_block4.close();
		sink[I::U64::Load32(management, env::detail::ContextAccess{}.contextAddress())];
		sink[I::Return()];

		_block8.close();
		sink[I::U64::Load(management, env::detail::ContextAccess{}.contextAddress())];

	}

	/* add the context-write function */
	{
		wasm::Prototype prototype = mod.prototype(u8"ctx_write_type", { { u8"offset", wasm::Type::i32 }, { u8"size", wasm::Type::i32 }, { u8"value", wasm::Type::i64 } }, {});
		wasm::Sink sink{ mod.function(u8"ctx_write", prototype, wasm::Export{}) };

		sink[I::Local::Get(sink.parameter(0))];
		sink[I::Local::Get(sink.parameter(2))];
		wasm::Block _block1{ sink, u8"size_1", { wasm::Type::i32, wasm::Type::i64 }, { wasm::Type::i32, wasm::Type::i64 } };
		wasm::Block _block2{ sink, u8"size_2", { wasm::Type::i32, wasm::Type::i64 }, { wasm::Type::i32, wasm::Type::i64 } };
		wasm::Block _block4{ sink, u8"size_4", { wasm::Type::i32, wasm::Type::i64 }, { wasm::Type::i32, wasm::Type::i64 } };
		wasm::Block _block8{ sink, u8"size_8", { wasm::Type::i32, wasm::Type::i64 }, { wasm::Type::i32, wasm::Type::i64 } };
		sink[I::Local::Get(sink.parameter(1))];
		sink[I::U32::TrailingNulls()];
		sink[I::Branch::Table({ _block1, _block2, _block4 }, _block8)];
		_block1.close();
		sink[I::U64::Store8(management, env::detail::ContextAccess{}.contextAddress())];
		sink[I::Return()];

		_block2.close();
		sink[I::U64::Store16(management, env::detail::ContextAccess{}.contextAddress())];
		sink[I::Return()];

		_block4.close();
		sink[I::U64::Store32(management, env::detail::ContextAccess{}.contextAddress())];
		sink[I::Return()];

		_block8.close();
		sink[I::U64::Store(management, env::detail::ContextAccess{}.contextAddress())];
	}

	/* add the exit-code-setter function */
	{
		wasm::Prototype prototype = mod.prototype(u8"ctx_set_exit_code_type", { { u8"code", wasm::Type::i32 } }, {});
		wasm::Sink sink{ mod.function(u8"ctx_set_exit_code", prototype, wasm::Export{}) };

		sink[I::Local::Get(sink.parameter(0))];
		sink[I::Call::Tail(pSetExit)];
	}
}
void trans::detail::ContextBuilder::setupBlockImports(wasm::Module& mod, const wasm::Memory& management, detail::ContextState& state) const {
	state.management = management;

	/* add the function-import for the exit-code-setter function */
	wasm::Prototype prototype = mod.prototype(u8"ctx_set_exit_code_type", { { u8"code", wasm::Type::i32 } }, {});
	state.exit = mod.function(u8"ctx_set_exit_code", prototype, wasm::Import{ u8"core" });
}
