#include "../env-process.h"
#include "blocks-bridge.h"

namespace I = wasm::inst;

env::Blocks::Blocks(env::Process* process) : pProcess{ process } {}

uint32_t env::Blocks::fLookup(env::guest_t address) const {
	pProcess->debug(str::Format<std::u8string>(u8"Lookup block: [{:#018x}]", address));
	return 0;
}
void env::Blocks::fAssociate(env::guest_t address, uint32_t index) {
	pProcess->debug(str::Format<std::u8string>(u8"Assigning [{:#018x}] to [{}]", address, index));
}
void env::Blocks::fFlushed() {
	pProcess->debug(u8"Flushed blocks");
}

void env::Blocks::setupCoreImports(wasm::Module& mod, env::CoreState& state) {
	/* add the import to the lookup-function */
	wasm::Prototype prototype = mod.prototype(u8"blocks_lookup_complex_type",
		{ { u8"process", wasm::Type::i64 }, { u8"addr", wasm::Type::i64 } },
		{ wasm::Type::i32 }
	);
	state.blocks_core.lookup = mod.function(u8"lookup_complex", prototype, wasm::Import{ u8"blocks" });

	/* add the import to the flush-function */
	prototype = mod.prototype(u8"blocks_flushed_type",
		{ { u8"process", wasm::Type::i64 } },
		{}
	);
	state.blocks_core.flushed = mod.function(u8"flushed", prototype, wasm::Import{ u8"blocks" });

	/* add the import to the associate-function */
	prototype = mod.prototype(u8"blocks_associate_type",
		{ { u8"process", wasm::Type::i64 }, { u8"addr", wasm::Type::i64 }, { u8"index", wasm::Type::i32 } },
		{}
	);
	state.blocks_core.associate = mod.function(u8"associate", prototype, wasm::Import{ u8"blocks" });

	/* allocate the cache-entries from the management memory */
	pCacheAddress = state.endOfManagement;
	state.endOfManagement += (1 << env::BlockLookupCacheBits) * uint32_t(sizeof(Blocks::BlockCache));
}
void env::Blocks::setupCoreBody(wasm::Module& mod, env::CoreState& state) const {
	/* add the function-table and total-count (first slot is always null-slot) */
	wasm::Table functions = mod.table(u8"function_list", true, wasm::Limit{ env::MinFunctionList });
	wasm::Global count = mod.global(u8"function_count", wasm::Type::i32, true);
	mod.value(count, wasm::Value::MakeU32(1));

	/* add the lookup function */
	wasm::Function execute;
	{
		wasm::Prototype prototype = mod.prototype(u8"blocks_goto_type", { { u8"addr", wasm::Type::i64 } }, {});
		execute = mod.function(u8"blocks_goto", prototype, wasm::Export{});
		wasm::Sink sink{ execute };
		wasm::Variable address = sink.parameter(0);
		wasm::Variable hashAddress = sink.local(wasm::Type::i32, u8"hash_address");
		wasm::Variable index = sink.local(wasm::Type::i32, u8"index");

		/* compute the address in the block-cache from the guest-address */
		sink[I::Local::Get(address)];
		sink[I::Local::Get(address)];
		sink[I::U64::Const(env::BlockLookupCacheBits)];
		sink[I::U64::ShiftRight()];
		sink[I::U64::XOr()];
		sink[I::U64::Shrink()];
		sink[I::U32::Const((1 << env::BlockLookupCacheBits) - 1)];
		sink[I::U32::And()];
		sink[I::U32::Const(sizeof(Blocks::BlockCache))];
		sink[I::U32::Mul()];
		sink[I::U32::Const(pCacheAddress)];
		sink[I::U32::Add()];
		sink[I::Local::Tee(hashAddress)];

		/* check if this is a cache-hit */
		sink[I::U64::Load(state.management, offsetof(Blocks::BlockCache, address))];
		sink[I::Local::Get(address)];
		sink[I::U64::Equal()];
		{
			/* fetch the current index and check if its actually valid */
			wasm::IfThen _if{ sink };
			sink[I::Local::Get(hashAddress)];
			sink[I::U32::Load(state.management, offsetof(Blocks::BlockCache, index))];
			sink[I::Local::Tee(index)];
			sink[I::U32::EqualZero()];
			sink[I::Branch::If(_if)];

			/* return/execute the function */
			sink[I::Local::Get(index)];
			sink[I::Call::IndirectTail(functions)];
		}

		/* perform a complex lookup */
		sink[I::U64::Const(pProcess)];
		sink[I::Local::Get(address)];
		sink[I::Call::Direct(state.blocks_core.lookup)];
		sink[I::Local::Tee(index)];

		/* check if the lookup failed and translate the function */
		sink[I::U32::EqualZero()];
		{
			/* perform the translation (tail-call to theoretically allow
			*	translate to immediately execute the upcoming block) */
			wasm::IfThen _if{ sink };
			sink[I::U64::Const(pProcess)];
			sink[I::Local::Get(address)];
			sink[I::Call::Tail(state.ctx_core.translate)];
		}

		/* write the index to the cache */
		sink[I::Local::Get(hashAddress)];
		sink[I::Local::Get(address)];
		sink[I::U64::Store(state.management, offsetof(Blocks::BlockCache, address))];
		sink[I::Local::Get(hashAddress)];
		sink[I::Local::Get(index)];
		sink[I::U32::Store(state.management, offsetof(Blocks::BlockCache, index))];

		/* return/execute the function */
		sink[I::Local::Get(index)];
		sink[I::Call::IndirectTail(functions)];
	}

	/* add the add-function function */
	{
		wasm::Prototype prototype = mod.prototype(u8"blocks_add_type", { { u8"function", wasm::Type::refFunction }, { u8"addr", wasm::Type::i64 } }, { wasm::Type::i32 });
		wasm::Sink sink{ mod.function(u8"blocks_add", prototype, wasm::Export{}) };
		wasm::Variable index = sink.local(wasm::Type::i32, u8"index");

		/* write the count to the index */
		sink[I::Global::Get(count)];
		sink[I::Local::Tee(index)];

		/* check if the table needs to be resized */
		sink[I::Table::Size(functions)];
		sink[I::U32::GreaterEqual()];
		{
			/* try to expand the table */
			wasm::IfThen _if0{ sink };
			sink[I::Ref::NullFunction()];
			sink[I::U32::Const(env::FunctionListGrowth)];
			sink[I::Table::Grow(functions)];

			/* check if the allocation failed and return 0 */
			sink[I::I32::Const(-1)];
			sink[I::U32::Equal()];
			wasm::IfThen _if1{ sink };
			sink[I::I32::Const(0)];
			sink[I::Return()];
		}

		/* write the function to the table */
		sink[I::Local::Get(index)];
		sink[I::Local::Get(sink.parameter(0))];
		sink[I::Table::Set(functions)];

		/* update the global counter */
		sink[I::Local::Get(index)];
		sink[I::U32::Const(1)];
		sink[I::U32::Add()];
		sink[I::Global::Set(count)];

		/* notify the main application about the association */
		sink[I::U64::Const(pProcess)];
		sink[I::Local::Get(sink.parameter(1))];
		sink[I::Local::Get(index)];
		sink[I::Call::Direct(state.blocks_core.associate)];

		/* return the 1 to indicate success */
		sink[I::U32::Const(1)];
	}

	/* add the blocks-execute function */
	{
		wasm::Prototype prototype = mod.prototype(u8"blocks_execute_type", { { u8"addr", wasm::Type::i64 } }, {});
		wasm::Sink sink{ mod.function(u8"blocks_execute", prototype, wasm::Export{}) };

		/* simply perform the goto */
		sink[I::Local::Get(sink.parameter(0))];
		sink[I::Call::Tail(execute)];
	}

	/* add the blocks-flushing function */
	{
		wasm::Sink sink{ mod.function(u8"blocks_flush_blocks", {}, {}, wasm::Export{}) };

		/* clear the caches */
		sink[I::U32::Const(pCacheAddress)];
		sink[I::U32::Const(0)];
		sink[I::U32::Const(uint64_t(1 << env::BlockLookupCacheBits) * sizeof(Blocks::BlockCache))];
		sink[I::Memory::Fill(state.management)];

		/* null the function-table (to ensure all references can be dropped) */
		sink[I::U32::Const(0)];
		sink[I::Ref::NullFunction()];
		sink[I::Global::Get(count)];
		sink[I::Table::Fill(functions)];

		/* reset the global counter */
		sink[I::U32::Const(1)];
		sink[I::Global::Set(count)];

		/* notify the main application about the flushing */
		sink[I::U64::Const(pProcess)];
		sink[I::Call::Direct(state.blocks_core.flushed)];
	}
}
void env::Blocks::setupBlockImports(wasm::Module& mod, env::BlockState& state) const {
	/* add the function-imports for the block-lookup */
	wasm::Prototype prototype = mod.prototype(u8"blocks_goto_type", { { u8"addr", wasm::Type::i64 } }, {});
	state.blocks.execute = mod.function(u8"blocks_goto", prototype, pProcess->context().imported());
}

void env::Blocks::execute(env::guest_t address) {
	bridge::Blocks::Execute(pProcess->context().id(), address);
}
void env::Blocks::flushBlocks() {
	bridge::Blocks::FlushBlocks(pProcess->context().id());
}
