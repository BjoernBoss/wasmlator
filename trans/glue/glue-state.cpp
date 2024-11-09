#include "glue-state.h"

namespace I = wasm::inst;

trans::detail::GlueState::GlueState(wasm::Module& mod) : pModule{ mod } {}
void trans::detail::GlueState::setup() {
	/* define the imports */
	wasm::Prototype prototype = pModule.prototype(u8"get_export_type", { { u8"ptr", wasm::Type::i32 }, { u8"size", wasm::Type::i32 } }, { wasm::Type::refFunction });
	pGetExport = pModule.function(u8"get_export", prototype, wasm::Import{ u8"host" });

	/* define the table and memory */
	pFunctions = pModule.table(u8"function_list", true);
	pMemory = pModule.memory(u8"memory", {}, wasm::Export{});
}
void trans::detail::GlueState::define(std::u8string_view name, std::initializer_list<wasm::Param> params, std::initializer_list<wasm::Type> result) {
	wasm::Prototype prototype = pModule.prototype(str::Build<std::u8string>(name, u8"_type"), params, result);

	/* define the temporary function */
	wasm::Sink sink{ pModule.function(name, prototype, wasm::Export{}) };

	/* write the parameter to the stack */
	for (uint32_t i = 0; i < params.size(); ++i)
		sink[I::Local::Get(sink.parameter(i))];
	sink[I::U32::Const(pLoaded.size())];
	sink[I::Call::IndirectTail(pFunctions, prototype)];

	/* write the name to the memory */
	pModule.data(pMemory, wasm::Value::MakeU32(pEndOfMemory), reinterpret_cast<const uint8_t*>(name.data()), name.size());
	uint32_t size = uint32_t(name.size());
	pLoaded.push_back({ pEndOfMemory, size });
	pEndOfMemory += size;
}
void trans::detail::GlueState::complete() {
	/* finalize the memory-limits */
	uint32_t pages = env::PhysPageCount(pEndOfMemory);
	pModule.limit(pMemory, wasm::Limit{ pages, pages });

	/* finalize the table-limits */
	uint32_t entries = uint32_t(pLoaded.size());
	pModule.limit(pFunctions, wasm::Limit{ entries, entries });

	/* add the setup-core-functions function */
	wasm::Prototype prototype = pModule.prototype(u8"setup_core_functions_type", {}, { wasm::Type::i32 });
	wasm::Sink sink{ pModule.function(u8"setup_core_functions", prototype, wasm::Export{}) };
	wasm::Variable func = sink.local(wasm::Type::refFunction, u8"exported_function");
	for (size_t i = 0; i < pLoaded.size(); ++i) {
		/* fetch the export from the core module */
		sink[I::U32::Const(pLoaded[i].name)];
		sink[I::U32::Const(pLoaded[i].size)];
		sink[I::Call::Direct(pGetExport)];
		sink[I::Local::Tee(func)];

		/* check if the loading failed and return 0 to indicate failure */
		sink[I::Ref::IsNull()];
		{
			wasm::IfThen _if{ sink };
			sink[I::U32::Const(0)];
			sink[I::Return()];
		}

		/* write the export to the table */
		sink[I::U32::Const(i)];
		sink[I::Local::Get(func)];
		sink[I::Table::Set(pFunctions)];
	}

	/* return 1 to indicate success*/
	sink[I::U32::Const(1)];
}
