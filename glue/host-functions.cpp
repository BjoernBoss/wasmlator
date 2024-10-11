#include "generate.h"

namespace I = wasm::inst;

void glue::SetupHostImports(glue::State& state) {
	/* add the load-core host import */
	wasm::Prototype hostLoadCoreType = state.module.prototype(u8"load_core_type",
		{ { u8"id", wasm::Type::i32 }, { u8"ptr", wasm::Type::i32 }, { u8"size", wasm::Type::i32 } },
		{}
	);
	state.hostLoadCore = state.module.function(u8"host_load_core", hostLoadCoreType, wasm::Import{ u8"host" });

	/* add the host-main-get function */
	wasm::Prototype hostGetMainExportType = state.module.prototype(u8"host_get_main_export_type",
		{ { u8"ptr", wasm::Type::i32 }, { u8"size", wasm::Type::i32 } },
		{ wasm::Type::refFunction }
	);
	state.hostGetMainFunction = state.module.function(u8"host_get_main_export", hostGetMainExportType, wasm::Import{ u8"host" });

	/* add the host-core-get function */
	wasm::Prototype hostGetCoreExportType = state.module.prototype(u8"host_get_core_export_type",
		{ { u8"instance", wasm::Type::refExtern }, { u8"ptr", wasm::Type::i32 }, { u8"size", wasm::Type::i32 } },
		{ wasm::Type::refFunction }
	);
	state.hostGetCoreFunction = state.module.function(u8"host_get_core_export", hostGetCoreExportType, wasm::Import{ u8"host" });
}

void glue::SetupHostBody(glue::State& state) {
	/* add the main-loaded callback function */
	if (true) {
		wasm::Prototype prototype = state.module.prototype(u8"host_main_loaded_type",
			{ wasm::Type::i32 },
			{}
		);
		wasm::Sink sink{ state.module.function(u8"host_main_loaded", prototype, wasm::Export{}) };

		/* check if the main application has already been loaded/failed */
		sink[I::Global::Get(state.mainLoaded)];
		sink[I::U32::Const(glue::MainState::notLoaded)];
		sink[I::U32::NotEqual()];
		{
			/* main is not awaited anymore, simply return */
			wasm::IfThen _if{ sink };
			sink[I::Return()];
		}

		/* check if the loading failed */
		sink[I::Local::Get(sink.parameter(0))];
		sink[I::U32::EqualZero()];
		{
			/* main failed to be loaded, update the state */
			wasm::IfThen _if{ sink };
			sink[I::U32::Const(glue::MainState::failed)];
			sink[I::Global::Set(state.mainLoaded)];
			sink[I::Return()];
		}

		/* add the code to fetch all of the separate functions and write them to the function table */
		for (size_t i = 0; i < size_t(glue::MainMapping::_count); ++i) {
			/* write the function-list index to the stack */
			sink[I::U32::Const(i)];

			/* load the address of the string */
			sink[I::U32::Const(state.mainStrings[i].offset)];

			/* load the size of the string */
			sink[I::U32::Const(state.mainStrings[i].length)];

			/* fetch the function-reference */
			sink[I::Call::Direct(state.hostGetMainFunction)];

			/* write the value to the function-list */
			sink[I::Table::Set(state.mainFunctions)];
		}

		/* update the state to mark the the main application as loaded */
		sink[I::U32::Const(glue::MainState::loaded)];
		sink[I::Global::Set(state.mainLoaded)];

		/* call the '_initialize' function of the main module (required for standalone wasm-modules without proper entry-point) */
		sink[I::U32::Const(glue::MainMapping::_initialize)];
		sink[I::Call::Indirect(state.mainFunctions, {}, {})];

		/* call the entry point of the main application */
		sink[I::U32::Const(glue::MainMapping::mainStartup)];
		sink[I::Call::IndirectTail(state.mainFunctions, {}, {})];
	}

	/* add the core-loaded callback function */
	if (true) {
		wasm::Prototype prototype = state.module.prototype(u8"host_core_loaded_type",
			{ { u8"id", wasm::Type::i32 }, { u8"instance", wasm::Type::refExtern } },
			{}
		);
		wasm::Sink sink{ state.module.function(u8"host_core_loaded", prototype, wasm::Export{}) };
		wasm::Variable listBase = sink.local(wasm::Type::i32, u8"list_base");

		/* check if the slot-state is valid */
		sink[I::Local::Get(sink.parameter(0))];
		sink[I::U32::Const(sizeof(glue::Slot))];
		sink[I::U32::Mul()];
		sink[I::U32::Load8(state.memory, state.addressOfList + offsetof(glue::Slot, state))];
		sink[I::U32::Const(glue::SlotState::loadingCore)];
		sink[I::U32::NotEqual()];
		{
			/* core is not awaited anymore, simply return */
			wasm::IfThen _if{ sink };
			sink[I::Return()];
		}

		/* check if the context loading failed */
		sink[I::Local::Get(sink.parameter(1))];
		sink[I::Ref::IsNull()];
		{
			wasm::IfThen _if{ sink };

			/* update the state as failed */
			sink[I::Local::Get(sink.parameter(0))];
			sink[I::U32::Const(sizeof(glue::Slot))];
			sink[I::U32::Mul()];
			sink[I::U32::Const(glue::SlotState::coreFailed)];
			sink[I::U32::Store8(state.memory, state.addressOfList + offsetof(glue::Slot, state))];

			/* notify the main application about the failure */
			sink[I::Local::Get(sink.parameter(0))];
			sink[I::U32::Const(sizeof(glue::Slot))];
			sink[I::U32::Mul()];
			sink[I::U64::Load(state.memory, state.addressOfList + offsetof(glue::Slot, process))];
			sink[I::U32::Const(0)];
			sink[I::U32::Const(glue::MainMapping::contextCoreLoaded)];
			sink[I::Call::IndirectTail(state.mainFunctions, { wasm::Type::i64, wasm::Type::i32 }, {})];
		}

		/* compute the list-base index */
		sink[I::Local::Get(sink.parameter(0))];
		sink[I::U32::Const(glue::CoreMapping::_count)];
		sink[I::U32::Mul()];
		sink[I::Local::Set(listBase)];

		/* store the core into the id-slot */
		sink[I::Local::Get(sink.parameter(0))];
		sink[I::Local::Get(sink.parameter(1))];
		sink[I::Table::Set(state.cores)];

		/* add the code to fetch all of the separate functions and write them to the import slots */
		for (size_t i = 0; i < size_t(glue::CoreMapping::_count); ++i) {
			/* write the function-list index to the stack */
			sink[I::Local::Get(listBase)];
			sink[I::U32::Const(i)];
			sink[I::U32::Add()];

			/* load the object reference */
			sink[I::Local::Get(sink.parameter(1))];

			/* load the address of the string */
			sink[I::U32::Const(state.coreStrings[i].offset)];

			/* load the size of the string */
			sink[I::U32::Const(state.coreStrings[i].length)];

			/* fetch the function-reference */
			sink[I::Call::Direct(state.hostGetCoreFunction)];

			/* write the value to the function-list */
			sink[I::Table::Set(state.coreFunctions)];
		}

		/* update the slot-state to mark the core as loaded */
		sink[I::Local::Get(sink.parameter(0))];
		sink[I::U32::Const(sizeof(glue::Slot))];
		sink[I::U32::Mul()];
		sink[I::U32::Const(glue::SlotState::coreLoaded)];
		sink[I::U32::Store8(state.memory, state.addressOfList + offsetof(glue::Slot, state))];

		/* notify the main application about the successful load */
		sink[I::Local::Get(sink.parameter(0))];
		sink[I::U32::Const(sizeof(glue::Slot))];
		sink[I::U32::Mul()];
		sink[I::U64::Load(state.memory, state.addressOfList + offsetof(glue::Slot, process))];
		sink[I::U32::Const(1)];
		sink[I::U32::Const(glue::MainMapping::contextCoreLoaded)];
		sink[I::Call::IndirectTail(state.mainFunctions, { wasm::Type::i64, wasm::Type::i32 }, {})];
	}
}
