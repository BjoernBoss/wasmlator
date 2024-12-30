#include "../generate.h"
#include "../environment/mapping/env-mapping.h"
#include "../environment/process/process-access.h"

void gen::detail::MappingBuilder::setupGlueMappings(detail::GlueState& glue) {
	glue.define(u8"map_reserve", { { u8"exports", wasm::Type::i32 } }, { wasm::Type::i32 });
	glue.define(u8"map_define", { { u8"name", wasm::Type::i32 }, { u8"size", wasm::Type::i32 }, { u8"address", wasm::Type::i64 } }, { wasm::Type::i32 });
	glue.define(u8"map_execute", { { u8"address", wasm::Type::i64 } }, { wasm::Type::i64 });
}
void gen::detail::MappingBuilder::setupCoreImports() {
	/* add the import to the lookup function */
	wasm::Prototype prototype = gen::Module->prototype(u8"main_resolve_type", { { u8"address", wasm::Type::i64 } }, { wasm::Type::i32 });
	pResolve = gen::Module->function(u8"main_resolve", prototype, wasm::Import{ u8"main" });

	/* add the import to the get-export function */
	prototype = gen::Module->prototype(u8"glue_get_function_type", { { u8"name", wasm::Type::i32 }, { u8"size", wasm::Type::i32 } }, { wasm::Type::refFunction });
	pGetFunction = gen::Module->function(u8"glue_get_function", prototype, wasm::Import{ u8"glue" });

	/* define the bindings passed to the blocks */
	env::detail::ProcessAccess::AddCoreBinding(u8"map", u8"map_functions");
	env::detail::ProcessAccess::AddCoreBinding(u8"map", u8"map_lookup");

	/* setup the block-type for the core */
	pBlockPrototype = gen::Module->prototype(u8"block_prototype", {}, { wasm::Type::i64 });
}
void gen::detail::MappingBuilder::setupCoreBody(const wasm::Memory& memory) const {
	detail::MappingState state;

	/* add the function-table and total-count (first slot is always null-slot to allow lookup-failure to be indicated by zero-return) */
	state.functions = gen::Module->table(u8"map_functions", true, wasm::Limit{ detail::MinFunctionList }, wasm::Export{});
	wasm::Global functionCount = gen::Module->global(u8"map_function_count", wasm::Type::i32, true);
	gen::Module->value(functionCount, wasm::Value::MakeU32(1));

	/* add the lookup function */
	{
		wasm::Prototype prototype = gen::Module->prototype(u8"map_lookup_type", { { u8"address", wasm::Type::i64 } }, { wasm::Type::i32 });
		state.lookup = gen::Module->function(u8"map_lookup", prototype, wasm::Export{});
		wasm::Sink sink{ state.lookup };
		wasm::Variable address = sink.param(0);
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
		sink[I::U32::Const(env::detail::MappingAccess::CacheAddress())];
		sink[I::U32::Add()];
		sink[I::Local::Tee(hashAddress)];

		/* check if this is a cache-hit */
		sink[I::U64::Load(memory, offsetof(env::detail::MappingCache, address))];
		sink[I::Local::Get(address)];
		sink[I::U64::Equal()];
		{
			/* fetch the current index and check if its actually valid */
			wasm::IfThen _if{ sink };
			sink[I::Local::Get(hashAddress)];
			sink[I::U32::Load(memory, offsetof(env::detail::MappingCache, index))];
			sink[I::Local::Tee(index)];
			sink[I::U32::EqualZero()];
			sink[I::Branch::If(_if)];

			/* return the index */
			sink[I::Local::Get(index)];
			sink[I::Return()];
		}

		/* resolve the actual address, which has not yet been cached (will either return a valid index or throw an exception) */
		sink[I::Local::Get(address)];
		sink[I::Call::Direct(pResolve)];
		sink[I::Local::Set(index)];

		/* write the index to the cache */
		sink[I::Local::Get(hashAddress)];
		sink[I::Local::Get(address)];
		sink[I::U64::Store(memory, offsetof(env::detail::MappingCache, address))];
		sink[I::Local::Get(hashAddress)];
		sink[I::Local::Get(index)];
		sink[I::U32::Store(memory, offsetof(env::detail::MappingCache, index))];

		/* return the function-index */
		sink[I::Local::Get(index)];
	}

	/* add the load-block function */
	{
		wasm::Prototype prototype = gen::Module->prototype(u8"map_reserve_type", { { u8"exports", wasm::Type::i32 } }, { wasm::Type::i32 });
		wasm::Sink sink{ gen::Module->function(u8"map_reserve", prototype, wasm::Export{}) };
		wasm::Variable finalCount = sink.local(wasm::Type::i32, u8"count");
		wasm::Variable tableSize = sink.local(wasm::Type::i32, u8"table_size");

		/* check if the functions-table needs to be resized to accommodate the exports */
		sink[I::Global::Get(functionCount)];
		sink[I::Param::Get(0)];
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

		/* return 1 to indicate success */
		sink[I::U32::Const(1)];
	}

	/* add the define function */
	{
		wasm::Prototype prototype = gen::Module->prototype(u8"map_define_type", { { u8"name", wasm::Type::i32 }, { u8"size", wasm::Type::i32 }, { u8"address", wasm::Type::i64 } }, { wasm::Type::i32 });
		wasm::Sink sink{ gen::Module->function(u8"map_define", prototype, wasm::Export{}) };
		wasm::Variable func = sink.local(wasm::Type::refFunction, u8"exported_function");
		wasm::Variable index = sink.local(wasm::Type::i32, u8"function_index");

		/* perform the load of the actual function */
		sink[I::Param::Get(0)];
		sink[I::Param::Get(1)];
		sink[I::Call::Direct(pGetFunction)];
		sink[I::Local::Tee(func)];

		/* check if the import failed */
		sink[I::Ref::IsNull()];
		{
			/* notify the application about the failure by returning the corresponding invalid-mapping index */
			wasm::IfThen _if{ sink };
			sink[I::U32::Const(env::detail::InvalidMapping)];
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
		wasm::Prototype prototype = gen::Module->prototype(u8"map_execute_type", { { u8"address", wasm::Type::i64 } }, { wasm::Type::i64 });
		wasm::Sink sink{ gen::Module->function(u8"map_execute", prototype, wasm::Export{}) };

		sink[I::Param::Get(0)];

		/* check if this is single-step mode, in which case only the single block
		*	needs to be executed, and otherwise loop until an exception is thrown */
		if (gen::Instance()->singleStep()) {

			/* perform the lookup of the address (will either find the index, or throw an exception) */
			sink[I::Call::Direct(state.lookup)];

			/* execute the address */
			sink[I::Call::IndirectTail(state.functions, pBlockPrototype)];
		}
		else {
			/* simply perform the lookup and execution until until an exception automatically exists the loop */
			wasm::Loop _loop{ sink, u8"exec_loop", { wasm::Type::i64 }, {} };

			/* perform the lookup of the address (will either find the index, or throw an exception) */
			sink[I::Call::Direct(state.lookup)];

			/* execute the address */
			sink[I::Call::Indirect(state.functions, pBlockPrototype)];

			/* loop back to the start of the loop (add unreachable, as loop is only exited via exceptions) */
			sink[I::Branch::Direct(_loop)];
			_loop.close();
			sink[I::Unreachable()];
		}
	}
}
void gen::detail::MappingBuilder::setupBlockImports(wasm::Prototype& blockPrototype, detail::MappingState& state) const {
	/* add the function-table import */
	state.functions = gen::Module->table(u8"map_functions", true, wasm::Limit{ detail::MinFunctionList }, wasm::Import{ u8"map" });

	/* add the function-import for the block-lookup */
	wasm::Prototype prototype = gen::Module->prototype(u8"map_lookup_type", { { u8"address", wasm::Type::i64 } }, { wasm::Type::i32 });
	state.lookup = gen::Module->function(u8"map_lookup", prototype, wasm::Import{ u8"map" });

	/* setup the general block-type */
	state.blockPrototype = (blockPrototype = gen::Module->prototype(u8"block_prototype", {}, { wasm::Type::i64 }));
}
