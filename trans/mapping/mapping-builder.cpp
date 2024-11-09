#include "mapping-builder.h"
#include "mapping-writer.h"

namespace I = wasm::inst;

void trans::detail::MappingBuilder::setupGlueMappings(detail::GlueState& glue) {
	glue.define(u8"map_load_block", { { u8"ptr", wasm::Type::i32 }, { u8"size", wasm::Type::i32 }, { u8"exports", wasm::Type::i32 } }, { wasm::Type::i32 });
	glue.define(u8"map_define", { { u8"name", wasm::Type::i32 }, { u8"size", wasm::Type::i32 }, { u8"address", wasm::Type::i64 } }, { wasm::Type::i32 });
	glue.define(u8"map_flush_blocks", {}, {});
	glue.define(u8"map_execute", { { u8"addr", wasm::Type::i64 } }, { wasm::Type::i32 });
	glue.define(u8"map_block_loaded", { { u8"instance", wasm::Type::refExtern } }, {});
}
void trans::detail::MappingBuilder::setupCoreImports(wasm::Module& mod) {
	/* add the import to the lookup function */
	wasm::Prototype prototype = mod.prototype(u8"main_resolve_type", { { u8"addr", wasm::Type::i64 } }, { wasm::Type::i32 });
	pResolve = mod.function(u8"main_resolve", prototype, wasm::Import{ u8"main" });

	/* add the import to the flush function */
	prototype = mod.prototype(u8"main_flushed_type", {}, {});
	pFlushed = mod.function(u8"main_flushed", prototype, wasm::Import{ u8"main" });

	/* add the import to the block-loaded function */
	prototype = mod.prototype(u8"main_block_loaded_type", { { u8"succeeded", wasm::Type::i32 } }, {});
	pBlockLoaded = mod.function(u8"main_block_loaded", prototype, wasm::Import{ u8"main" });

	/* add the import to the load-block function */
	prototype = mod.prototype(u8"host_load_block_type", { { u8"ptr", wasm::Type::i32 }, { u8"size", wasm::Type::i32 } }, {});
	pLoadBlock = mod.function(u8"host_load_block", prototype, wasm::Import{ u8"host" });

	/* add the import to the get-export function */
	prototype = mod.prototype(u8"host_get_export_type", { { u8"instance", wasm::Type::refExtern }, { u8"ptr", wasm::Type::i32 }, { u8"size", wasm::Type::i32 } }, { wasm::Type::refFunction });
	pGetExport = mod.function(u8"host_get_export", prototype, wasm::Import{ u8"host" });
}
void trans::detail::MappingBuilder::setupCoreBody(wasm::Module& mod, const wasm::Memory& management) const {
	detail::MappingState state;

	/* add the blocks-list and blocks-count */
	wasm::Table blocks = mod.table(u8"blocks_list", false, wasm::Limit{ detail::MinBlockList });
	wasm::Global blockCount = mod.global(u8"blocks_count", wasm::Type::i32, true);
	mod.value(blockCount, wasm::Value::MakeU32(0));

	/* add the function-table and total-count (first slot is always null-slot to allow lookup-failure to be indicated by zero-return) */
	state.functions = mod.table(u8"map_functions", true, wasm::Limit{ detail::MinFunctionList }, wasm::Export{});
	wasm::Global functionCount = mod.global(u8"map_function_count", wasm::Type::i32, true);
	mod.value(functionCount, wasm::Value::MakeU32(1));

	/* add the lookup function */
	{
		wasm::Prototype prototype = mod.prototype(u8"map_lookup_type", { { u8"addr", wasm::Type::i64 } }, { wasm::Type::i32 });
		state.lookup = mod.function(u8"map_lookup", prototype, wasm::Export{});
		wasm::Sink sink{ state.lookup };
		wasm::Variable address = sink.parameter(0);
		wasm::Variable hashAddress = sink.local(wasm::Type::i32, u8"hash_address");
		wasm::Variable index = sink.local(wasm::Type::i32, u8"index");

		/* compute the address in the block-cache from the guest-address */
		sink[I::Local::Get(address)];
		sink[I::Local::Get(address)];
		sink[I::U64::Const(env::detail::BlockLookupCacheBits)];
		sink[I::U64::ShiftRight()];
		sink[I::U64::XOr()];
		sink[I::U64::Shrink()];
		sink[I::U32::Const((1 << env::detail::BlockLookupCacheBits) - 1)];
		sink[I::U32::And()];
		sink[I::U32::Const(sizeof(env::detail::MappingCache))];
		sink[I::U32::Mul()];
		uint32_t cacheAddress = env::detail::MappingAccess{}.cacheAddress();
		if (cacheAddress > 0) {
			sink[I::U32::Const(cacheAddress)];
			sink[I::U32::Add()];
		}
		sink[I::Local::Tee(hashAddress)];

		/* check if this is a cache-hit */
		sink[I::U64::Load(management, offsetof(env::detail::MappingCache, address))];
		sink[I::Local::Get(address)];
		sink[I::U64::Equal()];
		{
			/* fetch the current index and check if its actually valid */
			wasm::IfThen _if{ sink };
			sink[I::Local::Get(hashAddress)];
			sink[I::U32::Load(management, offsetof(env::detail::MappingCache, index))];
			sink[I::Local::Tee(index)];
			sink[I::U32::EqualZero()];
			sink[I::Branch::If(_if)];

			/* return the index */
			sink[I::Local::Get(index)];
			sink[I::Return()];
		}

		/* resolve the actual address, which has not yet been cached */
		sink[I::Local::Get(address)];
		sink[I::Call::Direct(pResolve)];
		sink[I::Local::Tee(index)];

		/* check if the lookup failed and return the null-index */
		sink[I::U32::EqualZero()];
		{
			wasm::IfThen _if{ sink };

			sink[I::U32::Const(env::detail::NotFoundIndex)];
			sink[I::Return()];
		}

		/* write the index to the cache */
		sink[I::Local::Get(hashAddress)];
		sink[I::Local::Get(address)];
		sink[I::U64::Store(management, offsetof(env::detail::MappingCache, address))];
		sink[I::Local::Get(hashAddress)];
		sink[I::Local::Get(index)];
		sink[I::U32::Store(management, offsetof(env::detail::MappingCache, index))];

		/* return the function-index */
		sink[I::Local::Get(index)];
	}

	/* add the load-block function */
	{
		wasm::Prototype prototype = mod.prototype(u8"map_load_block_type", { { u8"ptr", wasm::Type::i32 }, { u8"size", wasm::Type::i32 }, { u8"exports", wasm::Type::i32 } }, { wasm::Type::i32 });
		wasm::Sink sink{ mod.function(u8"map_load_block", prototype, wasm::Export{}) };
		wasm::Variable finalCount = sink.local(wasm::Type::i32, u8"count");
		wasm::Variable tableSize = sink.local(wasm::Type::i32, u8"table_size");

		/* check if a load is currently in progress and fail */
		sink[I::U32::Const(env::detail::MappingAccess{}.loadingAddress())];
		sink[I::U32::Load(management)];
		sink[I::U32::Const(env::detail::LoadingState::ready)];
		sink[I::U32::NotEqual()];
		{
			wasm::IfThen _if{ sink };

			/* return 0 to indicate failure */
			sink[I::U32::Const(0)];
			sink[I::Return()];
		}

		/* check if the blocks-table needs to be resized */
		sink[I::Global::Get(blockCount)];
		sink[I::Table::Size(blocks)];
		sink[I::U32::Equal()];
		{
			/* try to expand the table */
			wasm::IfThen _if0{ sink };
			sink[I::Ref::NullExtern()];
			sink[I::U32::Const(detail::BlockListGrowth)];
			sink[I::Table::Grow(blocks)];

			/* check if the allocation failed and return 0 to indicate failure */
			sink[I::I32::Const(-1)];
			sink[I::U32::Equal()];
			wasm::IfThen _if1{ sink };
			sink[I::I32::Const(0)];
			sink[I::Return()];
		}

		/* check if the functions-table needs to be resized to accommodate the exports */
		sink[I::Global::Get(functionCount)];
		sink[I::Local::Get(sink.parameter(2))];
		sink[I::U32::Add()];
		sink[I::Local::Tee(finalCount)];
		sink[I::Table::Size(state.functions)];
		sink[I::Local::Tee(tableSize)];
		sink[I::U32::Greater()];
		{
			/* try to expand the table (at least by growth-number of entries) */
			wasm::IfThen _if0{ sink };
			sink[I::Ref::NullFunction()];
			sink[I::Local::Get(finalCount)];
			sink[I::Local::Get(tableSize)];
			sink[I::U32::Sub()];
			sink[I::Local::Tee(finalCount)];
			sink[I::U32::Const(detail::FunctionListGrowth)];
			sink[I::Local::Get(finalCount)];
			sink[I::U32::Const(detail::FunctionListGrowth)];
			sink[I::U32::GreaterEqual()];
			sink[I::Select()];
			sink[I::Table::Grow(state.functions)];

			/* check if the allocation failed and return 0 to indicate failure */
			sink[I::I32::Const(-1)];
			sink[I::U32::Equal()];
			wasm::IfThen _if1{ sink };
			sink[I::I32::Const(0)];
			sink[I::Return()];
		}

		/* mark the load as currently in progress */
		sink[I::U32::Const(env::detail::MappingAccess{}.loadingAddress())];
		sink[I::U32::Const(env::detail::LoadingState::busy)];
		sink[I::U32::Store(management)];

		/* perform the actual load-request */
		sink[I::Local::Get(sink.parameter(0))];
		sink[I::Local::Get(sink.parameter(1))];
		sink[I::Call::Direct(pLoadBlock)];

		/* return 1 to indicate success */
		sink[I::U32::Const(1)];
	}

	/* add the block-loaded function */
	{
		wasm::Prototype prototype = mod.prototype(u8"map_block_loaded_type", { { u8"instance", wasm::Type::refExtern } }, {});
		wasm::Sink sink{ mod.function(u8"map_block_loaded", prototype, wasm::Export{}) };

		/* check if the load failed and notify the main application */
		sink[I::Local::Get(sink.parameter(0))];
		sink[I::Ref::IsNull()];
		{
			wasm::IfThen _if{ sink };
			sink[I::U32::Const(0)];
			sink[I::Call::Tail(pBlockLoaded)];
		}

		/* write the instance to the last index of the blocks-list (must already be reserved, therefore no bounds-checking) */
		sink[I::Global::Get(blockCount)];
		sink[I::Local::Get(sink.parameter(0))];
		sink[I::Table::Set(blocks)];

		/* increment the block-counter */
		sink[I::Global::Get(blockCount)];
		sink[I::U32::Const(1)];
		sink[I::U32::Add()];
		sink[I::Global::Set(blockCount)];

		/* mark the load as completed */
		sink[I::U32::Const(env::detail::MappingAccess{}.loadingAddress())];
		sink[I::U32::Const(env::detail::LoadingState::ready)];
		sink[I::U32::Store(management)];

		/* notify the application about the completed load */
		sink[I::U32::Const(1)];
		sink[I::Call::Tail(pBlockLoaded)];
	}

	/* add the define function */
	{
		wasm::Prototype prototype = mod.prototype(u8"map_define_type", { { u8"name", wasm::Type::i32 }, { u8"size", wasm::Type::i32 }, { u8"addr", wasm::Type::i64 } }, { wasm::Type::i32 });
		wasm::Sink sink{ mod.function(u8"map_define", prototype, wasm::Export{}) };
		wasm::Variable func = sink.local(wasm::Type::refFunction, u8"exported_function");
		wasm::Variable index = sink.local(wasm::Type::i32, u8"function_index");

		/* fetch the reference to the last block */
		sink[I::Global::Get(blockCount)];
		sink[I::U32::Const(1)];
		sink[I::U32::Sub()];
		sink[I::Table::Get(blocks)];

		/* perform the load of the actual function */
		sink[I::Local::Get(sink.parameter(0))];
		sink[I::Local::Get(sink.parameter(1))];
		sink[I::Call::Direct(pGetExport)];
		sink[I::Local::Tee(func)];

		/* check if the import failed */
		sink[I::Ref::IsNull()];
		{
			/* notify the application about the failure by returning the corresponding not-found index */
			wasm::IfThen _if{ sink };
			sink[I::U32::Const(env::detail::NotFoundIndex)];
			sink[I::Return()];
		}

		/* write the instance to the last index of the functions-list (must already be reserved, therefore no bounds-checking) */
		sink[I::Global::Get(functionCount)];
		sink[I::Local::Tee(index)];
		sink[I::Local::Get(func)];
		sink[I::Table::Set(state.functions)];

		/* increment the functions-counter */
		sink[I::Local::Get(index)];
		sink[I::U32::Const(1)];
		sink[I::U32::Add()];
		sink[I::Global::Set(functionCount)];

		/* return the new index to the main application (will always be greater than zero, as first function-slot is reserved) */
		sink[I::Local::Get(index)];
	}

	/* add the blocks-execute function */
	{
		wasm::Prototype prototype = mod.prototype(u8"map_execute_type", { { u8"addr", wasm::Type::i64 } }, { wasm::Type::i32 });
		wasm::Sink sink{ mod.function(u8"map_execute", prototype, wasm::Export{}) };
		wasm::Variable result = sink.local(wasm::Type::i32, u8"result");

		sink[I::Local::Get(sink.parameter(0))];

		/* simply perform the lookup and execution until the result-code is not execute anymore */
		wasm::Loop _loop{ sink, u8"exec_loop", { wasm::Type::i64 }, {} };

		/* perform the lookup of the address */
		sink[I::Call::Direct(state.lookup)];
		sink[I::Local::Tee(result)];

		/* check if the address could not be resolved */
		detail::MappingWriter{ state, sink }.makeCheckFailed();
		{
			wasm::IfThen _if{ sink };

			sink[I::U32::Const(env::ExecState::translate)];
			sink[I::Return()];
		}

		/* execute the address */
		sink[I::Local::Get(result)];
		sink[I::Call::Indirect(state.functions, {}, { wasm::Type::i64, wasm::Type::i32 })];
		sink[I::Local::Tee(result)];

		/* check if the another execute has been encountered */
		sink[I::U32::Const(env::ExecState::_execute)];
		sink[I::U32::Equal()];
		sink[I::Branch::If(_loop)];

		/* return the exec-state */
		sink[I::Local::Get(result)];
		sink[I::Return()];
		_loop.close();
		sink[I::Unreachable()];
	}

	/* add the blocks-flushing function */
	{
		wasm::Sink sink{ mod.function(u8"map_flush_blocks", {}, {}, wasm::Export{}) };

		/* clear the caches */
		sink[I::U32::Const(env::detail::MappingAccess{}.cacheAddress())];
		sink[I::U32::Const(0)];
		sink[I::U32::Const(uint64_t(1 << env::detail::BlockLookupCacheBits) * sizeof(env::detail::MappingCache))];
		sink[I::Memory::Fill(management)];

		/* null the blocks-table (to ensure all references can be dropped) */
		sink[I::U32::Const(0)];
		sink[I::Ref::NullExtern()];
		sink[I::Global::Get(blockCount)];
		sink[I::Table::Fill(blocks)];

		/* null the function-table (to ensure all references can be dropped) */
		sink[I::U32::Const(0)];
		sink[I::Ref::NullFunction()];
		sink[I::Global::Get(functionCount)];
		sink[I::Table::Fill(state.functions)];

		/* reset the global counter */
		sink[I::U32::Const(0)];
		sink[I::Global::Set(blockCount)];
		sink[I::U32::Const(1)];
		sink[I::Global::Set(functionCount)];

		/* notify the main application about the flushing */
		sink[I::Call::Direct(pFlushed)];
	}
}
void trans::detail::MappingBuilder::setupBlockImports(wasm::Module& mod, detail::MappingState& state) const {
	/* add the function-table import */
	state.functions = mod.table(u8"map_functions", true, wasm::Limit{ detail::MinFunctionList }, wasm::Import{ u8"core" });

	/* add the function-import for the block-lookup */
	wasm::Prototype prototype = mod.prototype(u8"map_lookup_type", { { u8"addr", wasm::Type::i64 } }, { wasm::Type::i32 });
	state.lookup = mod.function(u8"map_lookup", prototype, wasm::Import{ u8"core" });
}
