#include "generate.h"

namespace I = wasm::inst;

static void SetupAccessFunctions(glue::State& state, wasm::Type type, std::u8string_view name, glue::CoreMapping read, glue::CoreMapping write, glue::CoreMapping execute) {
	/* add the read-implementation */
	if (true) {
		wasm::Sink sink{ state.module.function(str::Build<std::u8string>(u8"mem_read_", name), { wasm::Type::i32, wasm::Type::i64 }, { type }, wasm::Export{}) };

		/* fetch the parameter */
		sink[I::Local::Get(sink.parameter(1))];

		/* compute the index into the function list */
		sink[I::Local::Get(sink.parameter(0))];
		sink[I::U32::Const(glue::CoreMapping::_count)];
		sink[I::U32::Mul()];
		sink[I::U32::Const(read)];
		sink[I::U32::Add()];

		/* perform the actual call */
		sink[I::Call::IndirectTail(state.coreFunctions, { wasm::Type::i64 }, { type })];
	}

	/* add the write-implementation */
	if (true) {
		wasm::Sink sink{ state.module.function(str::Build<std::u8string>(u8"mem_write_", name), { wasm::Type::i32, wasm::Type::i64, type }, {}, wasm::Export{}) };

		/* fetch the parameter */
		sink[I::Local::Get(sink.parameter(1))];
		sink[I::Local::Get(sink.parameter(2))];

		/* compute the index into the function list */
		sink[I::Local::Get(sink.parameter(0))];
		sink[I::U32::Const(glue::CoreMapping::_count)];
		sink[I::U32::Mul()];
		sink[I::U32::Const(write)];
		sink[I::U32::Add()];

		/* perform the actual call */
		sink[I::Call::IndirectTail(state.coreFunctions, { wasm::Type::i64, type }, {})];
	}

	/* add the execute-implementation */
	if (true) {
		wasm::Sink sink{ state.module.function(str::Build<std::u8string>(u8"mem_execute_", name), { wasm::Type::i32, wasm::Type::i64 }, { type }, wasm::Export{}) };

		/* fetch the parameter */
		sink[I::Local::Get(sink.parameter(1))];

		/* compute the index into the function list */
		sink[I::Local::Get(sink.parameter(0))];
		sink[I::U32::Const(glue::CoreMapping::_count)];
		sink[I::U32::Mul()];
		sink[I::U32::Const(execute)];
		sink[I::U32::Add()];

		/* perform the actual call */
		sink[I::Call::IndirectTail(state.coreFunctions, { wasm::Type::i64 }, { type })];
	}
}

void glue::SetupMemoryFunctions(glue::State& state) {
	/* export the mem_expand_physical function and call the function from the corresponding core module */
	if (true) {
		wasm::Prototype prototype = state.module.prototype(u8"mem_expand_type",
			{ { u8"id", wasm::Type::i32 }, { u8"pages", wasm::Type::i32 } },
			{ wasm::Type::i32 }
		);
		wasm::Sink sink{ state.module.function(u8"mem_expand_physical", prototype, wasm::Export{}) };
		sink[I::Local::Get(sink.parameter(0))];
		sink[I::U32::Const(glue::CoreMapping::_count)];
		sink[I::U32::Mul()];
		sink[I::U32::Const(glue::CoreMapping::expandPhysical)];
		sink[I::U32::Add()];
		sink[I::Local::Get(sink.parameter(1))];
		sink[I::Call::IndirectTail(state.coreFunctions, { wasm::Type::i32 }, { wasm::Type::i32 })];
	}

	/* export the mem_move_physical function and call the function from the corresponding core module */
	if (true) {
		wasm::Prototype prototype = state.module.prototype(u8"mem_move_type",
			{ { u8"id", wasm::Type::i32 }, { u8"dest", wasm::Type::i32 }, { u8"source", wasm::Type::i32 }, { u8"size", wasm::Type::i32 } },
			{}
		);
		wasm::Sink sink{ state.module.function(u8"mem_move_physical", prototype, wasm::Export{}) };
		sink[I::Local::Get(sink.parameter(0))];
		sink[I::U32::Const(glue::CoreMapping::_count)];
		sink[I::U32::Mul()];
		sink[I::U32::Const(glue::CoreMapping::movePhysical)];
		sink[I::U32::Add()];
		sink[I::Local::Get(sink.parameter(1))];
		sink[I::Local::Get(sink.parameter(2))];
		sink[I::Local::Get(sink.parameter(3))];
		sink[I::Call::IndirectTail(state.coreFunctions, { wasm::Type::i32, wasm::Type::i32, wasm::Type::i32 }, {})];
	}

	/* export the flush_caches function and call the function from the corresponding core module */
	if (true) {
		wasm::Prototype prototype = state.module.prototype(u8"mem_flush_type",
			{ { u8"id", wasm::Type::i32 } },
			{}
		);
		wasm::Sink sink{ state.module.function(u8"mem_flush_caches", prototype, wasm::Export{}) };
		sink[I::Local::Get(sink.parameter(0))];
		sink[I::U32::Const(glue::CoreMapping::_count)];
		sink[I::U32::Mul()];
		sink[I::U32::Const(glue::CoreMapping::flushCaches)];
		sink[I::U32::Add()];
		sink[I::Call::IndirectTail(state.coreFunctions, {}, {})];
	}

	/* add all of the specialized memory-access functions */
	SetupAccessFunctions(state, wasm::Type::i32, u8"u8_i32", glue::CoreMapping::readi32Fromu8, glue::CoreMapping::writei32Fromu8, glue::CoreMapping::executei32Fromu8);
	SetupAccessFunctions(state, wasm::Type::i32, u8"i8_i32", glue::CoreMapping::readi32Fromi8, glue::CoreMapping::writei32Fromi8, glue::CoreMapping::executei32Fromi8);
	SetupAccessFunctions(state, wasm::Type::i32, u8"u16_i32", glue::CoreMapping::readi32Fromu16, glue::CoreMapping::writei32Fromu16, glue::CoreMapping::executei32Fromu16);
	SetupAccessFunctions(state, wasm::Type::i32, u8"i16_i32", glue::CoreMapping::readi32Fromi16, glue::CoreMapping::writei32Fromi16, glue::CoreMapping::executei32Fromi16);
	SetupAccessFunctions(state, wasm::Type::i32, u8"i32", glue::CoreMapping::readi32, glue::CoreMapping::writei32, glue::CoreMapping::executei32);
	SetupAccessFunctions(state, wasm::Type::i64, u8"i64", glue::CoreMapping::readi64, glue::CoreMapping::writei64, glue::CoreMapping::executei64);
	SetupAccessFunctions(state, wasm::Type::f32, u8"f32", glue::CoreMapping::readf32, glue::CoreMapping::writef32, glue::CoreMapping::executef32);
	SetupAccessFunctions(state, wasm::Type::f64, u8"f64", glue::CoreMapping::readf64, glue::CoreMapping::writef64, glue::CoreMapping::executef64);
}
