#include "glue-state.h"

namespace I = wasm::inst;

gen::detail::GlueState::GlueState(wasm::Module& mod) : pModule{ mod } {}
void gen::detail::GlueState::setup() {
	/* add the get-export import function */
	wasm::Prototype prototype = pModule.prototype(u8"host_get_function_type", { { u8"instance", wasm::Type::refExtern}, { u8"name", wasm::Type::i32 }, { u8"size", wasm::Type::i32 }, { u8"main_memory", wasm::Type::i32 } }, { wasm::Type::refFunction });
	pGetFunction = pModule.function(u8"host_get_function", prototype, wasm::Import{ u8"host" });

	/* add the get-function import function */
	prototype = pModule.prototype(u8"host_get_export_type", { { u8"instance", wasm::Type::refExtern}, { u8"name", wasm::Type::i32 }, { u8"size", wasm::Type::i32 } }, { wasm::Type::refExtern });
	pGetExport = pModule.function(u8"host_get_export", prototype, wasm::Import{ u8"host" });

	/* define the table and memory */
	pFunctions = pModule.table(u8"function_list", true);
	pMemory = pModule.memory(u8"memory", {}, wasm::Export{});
}
void gen::detail::GlueState::define(std::u8string_view name, std::initializer_list<wasm::Param> params, std::initializer_list<wasm::Type> result) {
	wasm::Prototype prototype = pModule.prototype(str::Build<std::u8string>(name, u8"_type"), params, result);

	/* define the temporary function */
	wasm::Sink sink{ pModule.function(name, prototype, wasm::Export{}) };

	/* write the parameter to the stack */
	for (uint32_t i = 0; i < params.size(); ++i)
		sink[I::Local::Get(sink.param(i))];
	sink[I::U32::Const(pLoaded.size())];
	sink[I::Call::IndirectTail(pFunctions, prototype)];

	/* write the name to the memory */
	pModule.data(pMemory, wasm::Value::MakeU32(pEndOfMemory), reinterpret_cast<const uint8_t*>(name.data()), name.size());
	uint32_t size = uint32_t(name.size());
	pLoaded.push_back({ pEndOfMemory, size });
	pEndOfMemory += size;
}
void gen::detail::GlueState::complete() {
	/* finalize the memory-limits */
	uint32_t pages = env::PhysPageCount(pEndOfMemory);
	pModule.limit(pMemory, wasm::Limit{ pages, pages });

	/* finalize the table-limits */
	uint32_t entries = uint32_t(pLoaded.size());
	pModule.limit(pFunctions, wasm::Limit{ entries, entries });

	/* setup the reference to the last instance */
	wasm::Global lastInstance = pModule.global(u8"last_instance", wasm::Type::refExtern, true);
	pModule.value(lastInstance, wasm::Value::MakeExtern());

	/* add the get-export function */
	{
		wasm::Prototype prototype = pModule.prototype(u8"glue_get_export_type", { { u8"name", wasm::Type::i32 }, { u8"size", wasm::Type::i32 } }, { wasm::Type::refExtern });
		wasm::Sink sink{ pModule.function(u8"glue_get_export", prototype, wasm::Export{}) };

		/* perform the core call using the last set instance */
		sink[I::Global::Get(lastInstance)];
		sink[I::Param::Get(0)];
		sink[I::Param::Get(1)];
		sink[I::Call::Tail(pGetExport)];
	}

	/* add the get-function function */
	{
		wasm::Prototype prototype = pModule.prototype(u8"glue_get_function_type", { { u8"name", wasm::Type::i32 }, { u8"size", wasm::Type::i32 } }, { wasm::Type::refFunction });
		wasm::Sink sink{ pModule.function(u8"glue_get_function", prototype, wasm::Export{}) };

		/* perform the core call using the last set instance */
		sink[I::Global::Get(lastInstance)];
		sink[I::Param::Get(0)];
		sink[I::Param::Get(1)];
		sink[I::U32::Const(1)];
		sink[I::Call::Tail(pGetFunction)];
	}

	/* add the setup-core-map function */
	{
		wasm::Prototype prototype = pModule.prototype(u8"glue_setup_core_map_type", {}, { wasm::Type::i32 });
		wasm::Sink sink{ pModule.function(u8"glue_setup_core_map", prototype, wasm::Export{}) };
		wasm::Variable func = sink.local(wasm::Type::refFunction, u8"exported_function");
		for (size_t i = 0; i < pLoaded.size(); ++i) {
			/* fetch the export from the core module */
			sink[I::Global::Get(lastInstance)];
			sink[I::U32::Const(pLoaded[i].name)];
			sink[I::U32::Const(pLoaded[i].size)];
			sink[I::U32::Const(0)];
			sink[I::Call::Direct(pGetFunction)];
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

	/* add the reset function (no need to reset last-instance, as the host should call it after being done with it) */
	{
		wasm::Prototype prototype = pModule.prototype(u8"glue_reset_type", {}, {});
		wasm::Sink sink{ pModule.function(u8"glue_reset", prototype, wasm::Export{}) };

		/* reset all references within the table */
		sink[I::U32::Const(0)];
		sink[I::Ref::NullFunction()];
		sink[I::U32::Const(entries)];
		sink[I::Table::Fill(pFunctions)];
	}

	/* add the set-last-instance function */
	{
		wasm::Prototype prototype = pModule.prototype(u8"set_last_instance_type", { { u8"instance", wasm::Type::refExtern } }, {});
		wasm::Sink sink{ pModule.function(u8"set_last_instance", prototype, wasm::Export{}) };

		/* write the reference to the global */
		sink[I::Param::Get(0)];
		sink[I::Global::Set(lastInstance)];
	}
}
