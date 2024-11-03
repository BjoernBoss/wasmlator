#include "mapping-builder.h"

namespace I = wasm::inst;

trans::detail::MappingBuilder::MappingBuilder(env::Process* process) : pProcess{ process } {}

void trans::detail::MappingBuilder::setupCoreImports(wasm::Module& mod, detail::MappingState& state) {
	/* add the import to the lookup-function */
	wasm::Prototype prototype = mod.prototype(u8"map_resolve_type",
		{ { u8"process", wasm::Type::i64 }, { u8"addr", wasm::Type::i64 } },
		{ wasm::Type::i32 }
	);
	pResolve = mod.function(u8"resolve", prototype, wasm::Import{ u8"map" });

	/* add the import to the flush-function */
	prototype = mod.prototype(u8"map_flushed_type",
		{ { u8"process", wasm::Type::i64 } },
		{}
	);
	pFlushed = mod.function(u8"flushed", prototype, wasm::Import{ u8"map" });

	/* add the import to the associate-function */
	prototype = mod.prototype(u8"map_associate_type",
		{ { u8"process", wasm::Type::i64 }, { u8"addr", wasm::Type::i64 }, { u8"index", wasm::Type::i32 } },
		{}
	);
	pAssociate = mod.function(u8"associate", prototype, wasm::Import{ u8"map" });
}
void trans::detail::MappingBuilder::setupCoreBody(wasm::Module& mod, detail::MappingState& state) const {
	/* add the blocks-list and blocks-count */
	wasm::Table blocks = mod.table(u8"blocks_list", false, wasm::Limit{ detail::MinBlockList });
	wasm::Global blockCount = mod.global(u8"blocks_count", wasm::Type::i32, true);
	mod.value(blockCount, wasm::Value::MakeU32(0));

	/* add the function-table and total-count (first slot is always null-slot) */
	state.functions = mod.table(u8"function_list", true, wasm::Limit{ detail::MinFunctionList }, wasm::Export{});
	wasm::Global functionCount = mod.global(u8"function_count", wasm::Type::i32, true);
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
		uint32_t cacheAddress = env::detail::MappingAccess{ pProcess }.cacheAddress();
		if (cacheAddress > 0) {
			sink[I::U32::Const(cacheAddress)];
			sink[I::U32::Add()];
		}
		sink[I::Local::Tee(hashAddress)];

		/* check if this is a cache-hit */
		sink[I::U64::Load(state.management, offsetof(env::detail::MappingCache, address))];
		sink[I::Local::Get(address)];
		sink[I::U64::Equal()];
		{
			/* fetch the current index and check if its actually valid */
			wasm::IfThen _if{ sink };
			sink[I::Local::Get(hashAddress)];
			sink[I::U32::Load(state.management, offsetof(env::detail::MappingCache, index))];
			sink[I::Local::Tee(index)];
			sink[I::U32::EqualZero()];
			sink[I::Branch::If(_if)];

			/* return the index */
			sink[I::Local::Get(index)];
			sink[I::Return()];
		}

		/* resolve the actual address, which has not yet been cached */
		sink[I::U64::Const(pProcess)];
		sink[I::Local::Get(address)];
		sink[I::Call::Direct(pResolve)];
		sink[I::Local::Tee(index)];

		/* check if the lookup failed and return the null-index */
		sink[I::U32::EqualZero()];
		{
			wasm::IfThen _if{ sink };

			sink[I::U32::Const(detail::NotFoundIndex)];
			sink[I::Return()];
		}

		/* write the index to the cache */
		sink[I::Local::Get(hashAddress)];
		sink[I::Local::Get(address)];
		sink[I::U64::Store(state.management, offsetof(env::detail::MappingCache, address))];
		sink[I::Local::Get(hashAddress)];
		sink[I::Local::Get(index)];
		sink[I::U32::Store(state.management, offsetof(env::detail::MappingCache, index))];

		/* return the function */
		sink[I::Local::Get(index)];
		sink[I::Table::Get(state.functions)];
	}

	/* add the reserve function */
	{
		wasm::Prototype prototype = mod.prototype(u8"map_reserve_type", { { u8"exports", wasm::Type::i32 } }, { wasm::Type::i32 });
		wasm::Sink sink{ mod.function(u8"map_reserve", prototype, wasm::Export{}) };
		wasm::Variable finalCount = sink.local(wasm::Type::i32, u8"count");
		wasm::Variable tableSize = sink.local(wasm::Type::i32, u8"table_size");

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

		/* check if the functions-table needs to be resized */
		sink[I::Global::Get(functionCount)];
		sink[I::Local::Get(sink.parameter(0))];
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

		/* return the 1 to indicate the reserving as successful */
		sink[I::U32::Const(1)];
	}

	/* add the block-loaded function */
	{
		wasm::Prototype prototype = mod.prototype(u8"map_block_loaded_type", { { u8"instance", wasm::Type::refExtern } }, {});
		wasm::Sink sink{ mod.function(u8"map_block_loaded", prototype, wasm::Export{}) };

		/* write the instance to the last index of the blocks-list (must already be reserved, therefore no bounds-checking) */
		sink[I::Global::Get(blockCount)];
		sink[I::Local::Get(sink.parameter(0))];
		sink[I::Table::Set(blocks)];

		/* increment the block-counter */
		sink[I::Global::Get(blockCount)];
		sink[I::U32::Const(1)];
		sink[I::U32::Add()];
		sink[I::Global::Set(blockCount)];
	}

	/* add the blocks-get-last function */
	{
		wasm::Prototype prototype = mod.prototype(u8"map_last_block_type", {}, { wasm::Type::refExtern });
		wasm::Sink sink{ mod.function(u8"map_last_block", prototype, wasm::Export{}) };

		/* fetch the last block */
		sink[I::Global::Get(blockCount)];
		sink[I::U32::Const(1)];
		sink[I::U32::Sub()];
		sink[I::Table::Get(blocks)];
	}

	/* add the add-function function */
	{
		wasm::Prototype prototype = mod.prototype(u8"map_add_export_type", { { u8"function", wasm::Type::refFunction }, { u8"addr", wasm::Type::i64 } }, {});
		wasm::Sink sink{ mod.function(u8"map_add_export", prototype, wasm::Export{}) };
		wasm::Variable index = sink.local(wasm::Type::i32, u8"index");

		/* write the instance to the last index of the functions-list (must already be reserved, therefore no bounds-checking) */
		sink[I::Global::Get(functionCount)];
		sink[I::Local::Tee(index)];
		sink[I::Local::Get(sink.parameter(0))];
		sink[I::Table::Set(state.functions)];

		/* increment the functions-counter */
		sink[I::Local::Get(index)];
		sink[I::U32::Const(1)];
		sink[I::U32::Add()];
		sink[I::Global::Set(functionCount)];

		/* notify the main application about the association */
		sink[I::U64::Const(pProcess)];
		sink[I::Local::Get(sink.parameter(1))];
		sink[I::Local::Get(index)];
		sink[I::Call::Direct(pAssociate)];
	}

	/* add the blocks-execute function */
	{
		wasm::Prototype prototype = mod.prototype(u8"map_execute_type", { { u8"addr", wasm::Type::i64 } }, {});
		wasm::Sink sink{ mod.function(u8"map_execute", prototype, wasm::Export{}) };

		/* simply perform the goto */
		sink[I::Local::Get(sink.parameter(0))];
		sink[I::Call::Tail(state.module.mapping.execute)];
	}

	/* add the blocks-flushing function */
	{
		wasm::Sink sink{ mod.function(u8"map_flush_blocks", {}, {}, wasm::Export{}) };

		/* clear the caches */
		sink[I::U32::Const(env::detail::MappingAccess{ pProcess }.cacheAddress())];
		sink[I::U32::Const(0)];
		sink[I::U32::Const(uint64_t(1 << env::detail::BlockLookupCacheBits) * sizeof(env::detail::MappingCache))];
		sink[I::Memory::Fill(state.management)];

		/* null the blocks-table (to ensure all references can be dropped) */
		sink[I::U32::Const(0)];
		sink[I::Ref::NullExtern()];
		sink[I::Global::Get(blockCount)];
		sink[I::Table::Fill(blocks)];

		/* reset the global counter */
		sink[I::U32::Const(1)];
		sink[I::Global::Set(functionCount)];

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
		sink[I::U64::Const(pProcess)];
		sink[I::Call::Direct(pFlushed)];
	}
}
void trans::detail::MappingBuilder::setupBlockImports(wasm::Module& mod, detail::MappingState& state) const {
	/* add the function-table import */
	state.functions = mod.table(u8"function_list", true, wasm::Limit{ detail::MinFunctionList }, wasm::Import{ u8"map" });

	/* add the function-import for the block-lookup */
	wasm::Prototype prototype = mod.prototype(u8"map_lookup_type", { { u8"addr", wasm::Type::i64 } }, { wasm::Type::i32 });
	state.lookup = mod.function(u8"lookup", prototype, wasm::Import{ u8"map" });
}
