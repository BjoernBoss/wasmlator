#include "wasm/wasm.h"
#include "writer/binary-writer.h"
#include "writer/text-writer.h"

enum class ExportIndex :uint8_t {
	expandPhysical,
	movePhysical,
	flushCaches,

	readi32Fromu8,
	readi32Fromi8,
	readi32Fromu16,
	readi32Fromi16,
	readi32,
	readi64,
	readf32,
	readf64,

	writei32Fromu8,
	writei32Fromi8,
	writei32Fromu16,
	writei32Fromi16,
	writei32,
	writei64,
	writef32,
	writef64,

	executei32Fromu8,
	executei32Fromi8,
	executei32Fromu16,
	executei32Fromi16,
	executei32,
	executei64,
	executef32,
	executef64,

	_count
};

namespace I = wasm::inst;

static void SetupHostImports(wasm::Module& mod) {
	/* add the load-core host import */
	wasm::Prototype hostLoadCoreType = mod.prototype(u8"load_core_type",
		{ { u8"id", wasm::Type::i32 }, { u8"ptr", wasm::Type::i32 }, { u8"size", wasm::Type::i32 } },
		{ wasm::Type::refExtern }
	);
	mod.function(u8"host_load_core", hostLoadCoreType, wasm::Import{ u8"host" });

	/* add the host-get function */
	wasm::Prototype hostGetFunctionType = mod.prototype(u8"get_function_type",
		{ { u8"object", wasm::Type::refExtern }, { u8"ptr", wasm::Type::i32 }, { u8"size", wasm::Type::i32 } },
		{ wasm::Type::refFunction }
	);
	mod.function(u8"host_get_function", hostGetFunctionType, wasm::Import{ u8"host" });
}

static wasm::Table SetupContextFunctions(wasm::Module& mod) {
	/* add the function table and the core-table */
	wasm::Table functions = mod.table(u8"function_list", true, wasm::Limit{ 2 * uint32_t(ExportIndex::_count) }, wasm::Export{});
	wasm::Table cores = mod.table(u8"core_list", false, wasm::Limit{ 2 }, wasm::Export{});

	/* add the host functions */
	{
		wasm::Prototype hostSetCoreType = mod.prototype(u8"host_core_callback_type",
			{ { u8"id", wasm::Type::i32 }, { u8"core", wasm::Type::refExtern } },
			{}
		);
		wasm::Sink sink{ mod.function(u8"host_core_callback", hostSetCoreType, wasm::Export{}) };

		/* store the core into the id-slot */
		sink[I::Local::Get(sink.parameter(0))];
		sink[I::Local::Get(sink.parameter(1))];
		sink[I::Table::Set(cores)];

		/* fetch all of the imports */
	}

	/* add the context functions */
	{
		wasm::Sink sink = { mod.function(u8"ctx_create", {}, { wasm::Type::i32 }, wasm::Export{}) };

		/* look for the first empty core-slot */

	}
	{
		wasm::Sink sink{ mod.function(u8"ctx_set_core", { wasm::Type::i32, wasm::Type::i32, wasm::Type::i32 }, {}, wasm::Export{}) };

		/* clear the core-entry */
		sink[I::Local::Get(sink.parameter(0))];
		sink[I::Ref::NullExtern()];
		sink[I::Table::Set(cores)];

		/* clear the import-list */
		sink[I::Local::Get(sink.parameter(0))];
		sink[I::U32::Const(ExportIndex::_count)];
		sink[I::U32::Mul()];
		sink[I::Ref::NullFunction()];
		sink[I::U32::Const(ExportIndex::_count)];
		sink[I::Table::Fill(functions)];
	}
	{
		wasm::Sink sink{ mod.function(u8"ctx_destroy", { wasm::Type::i32 }, {}, wasm::Export{}) };

		/* clear the core-entry */
		sink[I::Local::Get(sink.parameter(0))];
		sink[I::Ref::NullExtern()];
		sink[I::Table::Set(cores)];

		/* clear the import-list */
		sink[I::Local::Get(sink.parameter(0))];
		sink[I::U32::Const(ExportIndex::_count)];
		sink[I::U32::Mul()];
		sink[I::Ref::NullFunction()];
		sink[I::U32::Const(ExportIndex::_count)];
		sink[I::Table::Fill(functions)];
	}
	return functions;
}
static void SetupMemoryFunctions(wasm::Module& mod, wasm::Table& functions) {
	{
		wasm::Sink sink{ mod.function(u8"mem_expand_physical", { wasm::Type::i32, wasm::Type::i32 }, { wasm::Type::i32 }, wasm::Export{}) };
		sink[I::Local::Get(sink.parameter(0))];
		sink[I::U32::Const(ExportIndex::_count)];
		sink[I::U32::Mul()];
		sink[I::U32::Const(ExportIndex::expandPhysical)];
		sink[I::U32::Add()];
		sink[I::Local::Get(sink.parameter(1))];
		sink[I::Call::IndirectTail(functions, { wasm::Type::i32 }, { wasm::Type::i32 })];
	}

	{
		wasm::Sink sink{ mod.function(u8"mem_move_physical", { wasm::Type::i32, wasm::Type::i32, wasm::Type::i32, wasm::Type::i32 }, {}, wasm::Export{}) };
		sink[I::Local::Get(sink.parameter(0))];
		sink[I::U32::Const(ExportIndex::_count)];
		sink[I::U32::Mul()];
		sink[I::U32::Const(ExportIndex::movePhysical)];
		sink[I::U32::Add()];
		sink[I::Local::Get(sink.parameter(1))];
		sink[I::Local::Get(sink.parameter(2))];
		sink[I::Local::Get(sink.parameter(3))];
		sink[I::Call::IndirectTail(functions, { wasm::Type::i32, wasm::Type::i32, wasm::Type::i32 }, {})];
	}

	{
		wasm::Sink sink{ mod.function(u8"flush_caches", { wasm::Type::i32 }, {}, wasm::Export{}) };
		sink[I::Local::Get(sink.parameter(0))];
		sink[I::U32::Const(ExportIndex::_count)];
		sink[I::U32::Mul()];
		sink[I::U32::Const(ExportIndex::flushCaches)];
		sink[I::U32::Add()];
		sink[I::Call::IndirectTail(functions, {}, {})];
	}
}
static void SetupAccessFunctions(wasm::Module& mod, wasm::Table& functions, wasm::Type type, std::u8string_view name, ExportIndex read, ExportIndex write, ExportIndex execute) {
	{
		wasm::Sink sink{ mod.function(str::Build<std::u8string>(u8"mem_read_", name), { wasm::Type::i32, wasm::Type::i64 }, { type }, wasm::Export{}) };
		sink[I::Local::Get(sink.parameter(0))];
		sink[I::U32::Const(ExportIndex::_count)];
		sink[I::U32::Mul()];
		sink[I::U32::Const(read)];
		sink[I::U32::Add()];
		sink[I::Local::Get(sink.parameter(1))];
		sink[I::Call::IndirectTail(functions, { wasm::Type::i64 }, { type })];
	}

	{
		wasm::Sink sink{ mod.function(str::Build<std::u8string>(u8"mem_write_", name), { wasm::Type::i32, wasm::Type::i64, type }, {}, wasm::Export{}) };
		sink[I::Local::Get(sink.parameter(0))];
		sink[I::U32::Const(ExportIndex::_count)];
		sink[I::U32::Mul()];
		sink[I::U32::Const(write)];
		sink[I::U32::Add()];
		sink[I::Local::Get(sink.parameter(1))];
		sink[I::Local::Get(sink.parameter(2))];
		sink[I::Call::IndirectTail(functions, { wasm::Type::i64, type }, {})];
	}

	{
		wasm::Sink sink{ mod.function(str::Build<std::u8string>(u8"mem_execute_", name), { wasm::Type::i32, wasm::Type::i64 }, { type }, wasm::Export{}) };
		sink[I::Local::Get(sink.parameter(0))];
		sink[I::U32::Const(ExportIndex::_count)];
		sink[I::U32::Mul()];
		sink[I::U32::Const(execute)];
		sink[I::U32::Add()];
		sink[I::Local::Get(sink.parameter(1))];
		sink[I::Call::IndirectTail(functions, { wasm::Type::i64 }, { type })];
	}
}
static void SetupAllAccesses(wasm::Module& mod, wasm::Table& functions) {
	SetupAccessFunctions(mod, functions, wasm::Type::i32, u8"u8_i32", ExportIndex::readi32Fromu8, ExportIndex::writei32Fromu8, ExportIndex::executei32Fromu8);
	SetupAccessFunctions(mod, functions, wasm::Type::i32, u8"i8_i32", ExportIndex::readi32Fromi8, ExportIndex::writei32Fromi8, ExportIndex::executei32Fromi8);
	SetupAccessFunctions(mod, functions, wasm::Type::i32, u8"u16_i32", ExportIndex::readi32Fromu16, ExportIndex::writei32Fromu16, ExportIndex::executei32Fromu16);
	SetupAccessFunctions(mod, functions, wasm::Type::i32, u8"i16_i32", ExportIndex::readi32Fromi16, ExportIndex::writei32Fromi16, ExportIndex::executei32Fromi16);
	SetupAccessFunctions(mod, functions, wasm::Type::i32, u8"i32", ExportIndex::readi32, ExportIndex::writei32, ExportIndex::executei32);
	SetupAccessFunctions(mod, functions, wasm::Type::i64, u8"i64", ExportIndex::readi64, ExportIndex::writei64, ExportIndex::executei64);
	SetupAccessFunctions(mod, functions, wasm::Type::f32, u8"f32", ExportIndex::readf32, ExportIndex::writef32, ExportIndex::executef32);
	SetupAccessFunctions(mod, functions, wasm::Type::f64, u8"f64", ExportIndex::readf64, ExportIndex::writef64, ExportIndex::executef64);
}

static void SetupModule(wasm::ModuleInterface* writer) {
	wasm::Module mod{ writer };

	SetupHostImports(mod);
	wasm::Table functions = SetupContextFunctions(mod);

	SetupMemoryFunctions(mod, functions);
	SetupAllAccesses(mod, functions);
}

int main() {
	{
		writer::BinaryWriter _writer;

		/* produce the module and write it out */
		SetupModule(&_writer);
		const std::vector<uint8_t>& data = _writer.output();

		std::fstream _out{ "./host-module.wasm", std::ios::out | std::ios::binary };
		_out.write(reinterpret_cast<const char*>(data.data()), data.size());
	}

	{
		writer::TextWriter _writer;

		/* produce the module and write it out */
		SetupModule(&_writer);
		const std::u8string& data = _writer.output();

		std::fstream _out{ "./host-module.wat", std::ios::out | std::ios::binary };
		_out.write(reinterpret_cast<const char*>(data.data()), data.size());
	}
	return 0;
}
