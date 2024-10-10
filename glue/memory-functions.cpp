#include "generate.h"

namespace I = wasm::inst;

static void SetupAccessFunctions(glue::State& state, wasm::Type type, std::u8string_view name, glue::Mapping read, glue::Mapping write, glue::Mapping execute) {
	/* add the read-implementation */
	{
		wasm::Sink sink{ state.module.function(str::Build<std::u8string>(u8"mem_read_", name), { wasm::Type::i32, wasm::Type::i64 }, { type }, wasm::Export{}) };

		/* fetch the parameter */
		sink[I::Local::Get(sink.parameter(1))];

		/* compute the index into the function list */
		sink[I::Local::Get(sink.parameter(0))];
		sink[I::U32::Const(glue::Mapping::_count)];
		sink[I::U32::Mul()];
		sink[I::U32::Const(read)];
		sink[I::U32::Add()];

		/* perform the actual call */
		sink[I::Call::IndirectTail(state.functions, { wasm::Type::i64 }, { type })];
	}

	/* add the write-implementation */
	{
		wasm::Sink sink{ state.module.function(str::Build<std::u8string>(u8"mem_write_", name), { wasm::Type::i32, wasm::Type::i64, type }, {}, wasm::Export{}) };

		/* fetch the parameter */
		sink[I::Local::Get(sink.parameter(1))];
		sink[I::Local::Get(sink.parameter(2))];

		/* compute the index into the function list */
		sink[I::Local::Get(sink.parameter(0))];
		sink[I::U32::Const(glue::Mapping::_count)];
		sink[I::U32::Mul()];
		sink[I::U32::Const(write)];
		sink[I::U32::Add()];

		/* perform the actual call */
		sink[I::Call::IndirectTail(state.functions, { wasm::Type::i64, type }, {})];
	}

	/* add the execute-implementation */
	{
		wasm::Sink sink{ state.module.function(str::Build<std::u8string>(u8"mem_execute_", name), { wasm::Type::i32, wasm::Type::i64 }, { type }, wasm::Export{}) };

		/* fetch the parameter */
		sink[I::Local::Get(sink.parameter(1))];

		/* compute the index into the function list */
		sink[I::Local::Get(sink.parameter(0))];
		sink[I::U32::Const(glue::Mapping::_count)];
		sink[I::U32::Mul()];
		sink[I::U32::Const(execute)];
		sink[I::U32::Add()];

		/* perform the actual call */
		sink[I::Call::IndirectTail(state.functions, { wasm::Type::i64 }, { type })];
	}
}

void glue::SetupMemoryFunctions(glue::State& state) {
	/* export the mem_expand_physical function and call the function from the corresponding core module */
	{
		wasm::Sink sink{ state.module.function(u8"mem_expand_physical", { wasm::Type::i32, wasm::Type::i32 }, { wasm::Type::i32 }, wasm::Export{}) };
		sink[I::Local::Get(sink.parameter(0))];
		sink[I::U32::Const(glue::Mapping::_count)];
		sink[I::U32::Mul()];
		sink[I::U32::Const(glue::Mapping::expandPhysical)];
		sink[I::U32::Add()];
		sink[I::Local::Get(sink.parameter(1))];
		sink[I::Call::IndirectTail(state.functions, { wasm::Type::i32 }, { wasm::Type::i32 })];
	}

	/* export the mem_move_physical function and call the function from the corresponding core module */
	{
		wasm::Sink sink{ state.module.function(u8"mem_move_physical", { wasm::Type::i32, wasm::Type::i32, wasm::Type::i32, wasm::Type::i32 }, {}, wasm::Export{}) };
		sink[I::Local::Get(sink.parameter(0))];
		sink[I::U32::Const(glue::Mapping::_count)];
		sink[I::U32::Mul()];
		sink[I::U32::Const(glue::Mapping::movePhysical)];
		sink[I::U32::Add()];
		sink[I::Local::Get(sink.parameter(1))];
		sink[I::Local::Get(sink.parameter(2))];
		sink[I::Local::Get(sink.parameter(3))];
		sink[I::Call::IndirectTail(state.functions, { wasm::Type::i32, wasm::Type::i32, wasm::Type::i32 }, {})];
	}

	/* export the flush_caches function and call the function from the corresponding core module */
	{
		wasm::Sink sink{ state.module.function(u8"mem_flush_caches", { wasm::Type::i32 }, {}, wasm::Export{}) };
		sink[I::Local::Get(sink.parameter(0))];
		sink[I::U32::Const(glue::Mapping::_count)];
		sink[I::U32::Mul()];
		sink[I::U32::Const(glue::Mapping::flushCaches)];
		sink[I::U32::Add()];
		sink[I::Call::IndirectTail(state.functions, {}, {})];
	}

	/* add all of the specialized memory-access functions */
	SetupAccessFunctions(state, wasm::Type::i32, u8"u8_i32", glue::Mapping::readi32Fromu8, glue::Mapping::writei32Fromu8, glue::Mapping::executei32Fromu8);
	SetupAccessFunctions(state, wasm::Type::i32, u8"i8_i32", glue::Mapping::readi32Fromi8, glue::Mapping::writei32Fromi8, glue::Mapping::executei32Fromi8);
	SetupAccessFunctions(state, wasm::Type::i32, u8"u16_i32", glue::Mapping::readi32Fromu16, glue::Mapping::writei32Fromu16, glue::Mapping::executei32Fromu16);
	SetupAccessFunctions(state, wasm::Type::i32, u8"i16_i32", glue::Mapping::readi32Fromi16, glue::Mapping::writei32Fromi16, glue::Mapping::executei32Fromi16);
	SetupAccessFunctions(state, wasm::Type::i32, u8"i32", glue::Mapping::readi32, glue::Mapping::writei32, glue::Mapping::executei32);
	SetupAccessFunctions(state, wasm::Type::i64, u8"i64", glue::Mapping::readi64, glue::Mapping::writei64, glue::Mapping::executei64);
	SetupAccessFunctions(state, wasm::Type::f32, u8"f32", glue::Mapping::readf32, glue::Mapping::writef32, glue::Mapping::executef32);
	SetupAccessFunctions(state, wasm::Type::f64, u8"f64", glue::Mapping::readf64, glue::Mapping::writef64, glue::Mapping::executef64);
}
