#include "generate.h"

namespace I = wasm::inst;

void glue::SetupHostImports(glue::State& state) {
	/* add the load-core host import */
	wasm::Prototype hostLoadCoreType = state.module.prototype(u8"load_core_type",
		{ { u8"id", wasm::Type::i32 }, { u8"ptr", wasm::Type::i32 }, { u8"size", wasm::Type::i32 } },
		{}
	);
	state.hostLoadCore = state.module.function(u8"host_load_core", hostLoadCoreType, wasm::Import{ u8"host" });

	/* add the host-get function */
	wasm::Prototype hostGetFunctionType = state.module.prototype(u8"get_function_type",
		{ { u8"object", wasm::Type::refExtern }, { u8"ptr", wasm::Type::i32 }, { u8"size", wasm::Type::i32 } },
		{ wasm::Type::refFunction }
	);
	state.hostGetFunction = state.module.function(u8"host_get_function", hostGetFunctionType, wasm::Import{ u8"host" });
}

void glue::SetupHostBody(glue::State& state) {
	/* add the host-callback function */
	{
		wasm::Prototype hostSetCoreType = state.module.prototype(u8"host_core_callback_type",
			{ { u8"id", wasm::Type::i32 }, { u8"core", wasm::Type::refExtern } },
			{}
		);
		wasm::Sink sink{ state.module.function(u8"host_core_callback", hostSetCoreType, wasm::Export{}) };
		wasm::Variable listBase = sink.local(wasm::Type::i32, u8"list_base");

		/* check if the slot-state is valid */
		sink[I::Local::Get(sink.parameter(0))];
		sink[I::U32::Load8(state.memory, state.addressOfList)];
		sink[I::U32::Const(glue::SlotState::loadingCore)];
		sink[I::U32::NotEqual()];
		{
			/* core is not awaited anymore, simply return */
			wasm::IfThen _if{ sink };
			sink[I::Return()];
		}

		/* compute the list-base index */
		sink[I::Local::Get(sink.parameter(0))];
		sink[I::U32::Const(glue::Mapping::_count)];
		sink[I::U32::Mul()];
		sink[I::Local::Set(listBase)];

		/* store the core into the id-slot */
		sink[I::Local::Get(sink.parameter(0))];
		sink[I::Local::Get(sink.parameter(1))];
		sink[I::Table::Set(state.cores)];

		/* add the code to fetch all of the separate functions and write them to the import slots */
		for (size_t i = 0; i < size_t(glue::Mapping::_count); ++i) {
			/* write the function-list index to the stack */
			sink[I::Local::Get(listBase)];
			sink[I::U32::Const(i)];
			sink[I::U32::Add()];

			/* load the object reference */
			sink[I::Local::Get(sink.parameter(1))];

			/* load the address of the string */
			sink[I::U32::Const(state.strings[i].offset)];

			/* load the size of the string */
			sink[I::U32::Const(state.strings[i].length)];

			/* fetch the function-reference */
			sink[I::Call::Direct(state.hostGetFunction)];

			/* write the value to the function-list */
			sink[I::Table::Set(state.functions)];
		}

		/* update the slot-state to mark the core as loaded */
		sink[I::Local::Get(sink.parameter(0))];
		sink[I::U32::Const(glue::SlotState::coreLoaded)];
		sink[I::U32::Store8(state.memory, state.addressOfList)];
	}
}
