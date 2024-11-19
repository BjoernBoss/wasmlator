#include "context-builder.h"
#include "../environment/context/context-access.h"
#include "../environment/process/process-access.h"

namespace I = wasm::inst;

void gen::detail::ContextBuilder::setupGlueMappings(detail::GlueState& glue) {
	glue.define(u8"ctx_read", { { u8"offset", wasm::Type::i32 }, { u8"size", wasm::Type::i32 } }, { wasm::Type::i64 });
	glue.define(u8"ctx_write", { { u8"offset", wasm::Type::i32 }, { u8"size", wasm::Type::i32 }, { u8"value", wasm::Type::i64 } }, {});
}
void gen::detail::ContextBuilder::setupCoreImports(wasm::Module& mod) const {
	/* import the main terminate method and pass it through as binding to the blocks */
	wasm::Prototype prototype = mod.prototype(u8"main_terminate_type", { { u8"code", wasm::Type::i32 } }, {});
	mod.function(u8"main_terminate", prototype, wasm::Transport{ u8"main" });
	env::detail::ProcessAccess::AddCoreBinding(u8"ctx", u8"main_terminate");

	/* import the main not decodable method and pass it through as binding to the blocks */
	prototype = mod.prototype(u8"main_not_decodable_type", { { u8"address", wasm::Type::i64 } }, {});
	mod.function(u8"main_not_decodable", prototype, wasm::Transport{ u8"main" });
	env::detail::ProcessAccess::AddCoreBinding(u8"ctx", u8"main_not_decodable");

	/* import the main not reachable method and pass it through as binding to the blocks */
	prototype = mod.prototype(u8"main_not_reachable_type", { { u8"address", wasm::Type::i64 } }, {});
	mod.function(u8"main_not_reachable", prototype, wasm::Transport{ u8"main" });
	env::detail::ProcessAccess::AddCoreBinding(u8"ctx", u8"main_not_reachable");
}
void gen::detail::ContextBuilder::setupCoreBody(wasm::Module& mod, const wasm::Memory& management) const {
	/* add the context-read function */
	{
		wasm::Prototype prototype = mod.prototype(u8"ctx_read_type", { { u8"offset", wasm::Type::i32 }, { u8"size", wasm::Type::i32 } }, { wasm::Type::i64 });
		wasm::Sink sink{ mod.function(u8"ctx_read", prototype, wasm::Export{}) };

		sink[I::Param::Get(0)];
		wasm::Block _block8{ sink, u8"size_8", { wasm::Type::i32 }, { wasm::Type::i32 } };
		wasm::Block _block4{ sink, u8"size_4", { wasm::Type::i32 }, { wasm::Type::i32 } };
		wasm::Block _block2{ sink, u8"size_2", { wasm::Type::i32 }, { wasm::Type::i32 } };
		wasm::Block _block1{ sink, u8"size_1", { wasm::Type::i32 }, { wasm::Type::i32 } };
		sink[I::Param::Get(1)];
		sink[I::U32::TrailingNulls()];
		sink[I::Branch::Table({ _block1, _block2, _block4 }, _block8)];
		_block1.close();
		sink[I::U64::Load8(management, env::detail::ContextAccess::ContextAddress())];
		sink[I::Return()];

		_block2.close();
		sink[I::U64::Load16(management, env::detail::ContextAccess::ContextAddress())];
		sink[I::Return()];

		_block4.close();
		sink[I::U64::Load32(management, env::detail::ContextAccess::ContextAddress())];
		sink[I::Return()];

		_block8.close();
		sink[I::U64::Load(management, env::detail::ContextAccess::ContextAddress())];

	}

	/* add the context-write function */
	{
		wasm::Prototype prototype = mod.prototype(u8"ctx_write_type", { { u8"offset", wasm::Type::i32 }, { u8"size", wasm::Type::i32 }, { u8"value", wasm::Type::i64 } }, {});
		wasm::Sink sink{ mod.function(u8"ctx_write", prototype, wasm::Export{}) };

		sink[I::Param::Get(0)];
		sink[I::Param::Get(2)];
		wasm::Block _block8{ sink, u8"size_8", { wasm::Type::i32, wasm::Type::i64 }, { wasm::Type::i32, wasm::Type::i64 } };
		wasm::Block _block4{ sink, u8"size_4", { wasm::Type::i32, wasm::Type::i64 }, { wasm::Type::i32, wasm::Type::i64 } };
		wasm::Block _block2{ sink, u8"size_2", { wasm::Type::i32, wasm::Type::i64 }, { wasm::Type::i32, wasm::Type::i64 } };
		wasm::Block _block1{ sink, u8"size_1", { wasm::Type::i32, wasm::Type::i64 }, { wasm::Type::i32, wasm::Type::i64 } };
		sink[I::Param::Get(1)];
		sink[I::U32::TrailingNulls()];
		sink[I::Branch::Table({ _block1, _block2, _block4 }, _block8)];
		_block1.close();
		sink[I::U64::Store8(management, env::detail::ContextAccess::ContextAddress())];
		sink[I::Return()];

		_block2.close();
		sink[I::U64::Store16(management, env::detail::ContextAccess::ContextAddress())];
		sink[I::Return()];

		_block4.close();
		sink[I::U64::Store32(management, env::detail::ContextAccess::ContextAddress())];
		sink[I::Return()];

		_block8.close();
		sink[I::U64::Store(management, env::detail::ContextAccess::ContextAddress())];
	}
}
void gen::detail::ContextBuilder::setupBlockImports(wasm::Module& mod, const wasm::Memory& management, detail::ContextShared& shared, detail::ContextState& state) const {
	state.management = management;

	/* add the function-import for the terminate function */
	wasm::Prototype prototype = mod.prototype(u8"main_terminate_type", { { u8"code", wasm::Type::i32 } }, {});
	state.terminate = mod.function(u8"main_terminate", prototype, wasm::Import{ u8"ctx" });

	/* add the function-import for the not decodable function */
	prototype = mod.prototype(u8"main_not_decodable_type", { { u8"address", wasm::Type::i64 } }, {});
	shared.notDecodable = mod.function(u8"main_not_decodable", prototype, wasm::Import{ u8"ctx" });

	/* add the function-import for the not reachable function */
	prototype = mod.prototype(u8"main_not_reachable_type", { { u8"address", wasm::Type::i64 } }, {});
	shared.notReachable = mod.function(u8"main_not_reachable", prototype, wasm::Import{ u8"ctx" });
}
