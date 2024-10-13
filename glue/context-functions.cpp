#include "generate.h"

namespace I = wasm::inst;

void glue::SetupContextFunctions(glue::State& state) {
	/* add the context-create function */
	if (true) {
		wasm::Prototype prototype = state.module.prototype(u8"ctx_create_type",
			{ { u8"process", wasm::Type::i64 } },
			{ wasm::Type::i32 }
		);
		wasm::Sink sink = { state.module.function(u8"ctx_create", prototype, wasm::Export{}) };
		wasm::Variable index = sink.local(wasm::Type::i32, u8"index");
		wasm::Variable slotAddress = sink.local(wasm::Type::i32, u8"slot_address");

		/* check if the main application has been loaded */
		sink[I::Global::Get(state.mainLoaded)];
		sink[I::U32::Const(glue::MainState::loaded)];
		sink[I::U32::NotEqual()];
		{
			/* main has not yet been loaded, fail creation */
			wasm::IfThen _if{ sink };
			sink[I::U32::Const(0)];
			sink[I::Return()];
		}

		/* initialize the slot address */
		sink[I::U32::Const(state.addressOfList)];
		sink[I::Local::Set(slotAddress)];

		/* iterate over the slots and look for the first available entry */
		{
			wasm::Block _exit{ sink, u8"exit_empty_slot_loop" };
			wasm::Loop _loop{ sink, u8"empty_slot_loop" };

			/* check if the end of the loop has been found */
			sink[I::Local::Get(index)];
			sink[I::Global::Get(state.slotCount)];
			sink[I::U32::GreaterEqual()];
			sink[I::Branch::If(_exit)];

			/* load the current state  */
			sink[I::Local::Get(slotAddress)];
			sink[I::U32::Load8(state.memory, offsetof(glue::Slot, state))];

			/* check if an index has been found */
			sink[I::U32::Const(glue::SlotState::available)];
			sink[I::U32::NotEqual()];
			{
				wasm::IfThen _if{ sink };

				/* advance the index and slot-address */
				sink[I::Local::Get(index)];
				sink[I::U32::Const(1)];
				sink[I::U32::Add()];
				sink[I::Local::Set(index)];
				sink[I::Local::Get(slotAddress)];
				sink[I::U32::Const(sizeof(glue::Slot))];
				sink[I::U32::Add()];
				sink[I::Local::Set(slotAddress)];

				/* go back to the loop header */
				sink[I::Branch::Direct(_loop)];
			}

			/* mark the index as allocated */
			sink[I::Local::Get(slotAddress)];
			sink[I::U32::Const(glue::SlotState::awaitingCore)];
			sink[I::U32::Store8(state.memory, offsetof(glue::Slot, state))];

			/* write the process-value out */
			sink[I::Local::Get(slotAddress)];
			sink[I::Local::Get(sink.parameter(0))];
			sink[I::U64::Store(state.memory, offsetof(glue::Slot, process))];

			/* return the index (no need to alloate core/function slots as this is a
			*	reused context-index, hence the allocations have already occurred) */
			sink[I::Local::Get(index)];
			sink[I::Return()];
		}

		/* no entry has been found, try to expand the usage-list byte the size of one slot */
		sink[I::Local::Get(slotAddress)];
		sink[I::U32::Const(sizeof(glue::Slot))];
		sink[I::U32::Add()];
		sink[I::Memory::Size(state.memory)];
		sink[I::U32::Const(glue::PageSize)];
		sink[I::U32::Mul()];
		sink[I::U32::Greater()];
		{
			/* memory needs to be expanded */
			wasm::IfThen _if0{ sink };
			sink[I::U32::Const((sizeof(glue::Slot) + glue::PageSize - 1) / glue::PageSize)];
			sink[I::Memory::Grow(state.memory)];

			/* check if the allocation failed and return 0 */
			sink[I::I32::Const(-1)];
			sink[I::U32::Equal()];
			wasm::IfThen _if1{ sink };
			sink[I::U32::Const(0)];
			sink[I::Return()];
		}

		/* try to grow the function-list (check its size as only the last allocation might
		*	have failed and hence the first list might already have enough space) */
		sink[I::Global::Get(state.slotCount)];
		sink[I::U32::Const(glue::CoreMapping::_count)];
		sink[I::U32::Mul()];
		sink[I::Table::Size(state.coreFunctions)];
		sink[I::U32::GreaterEqual()];
		{
			/* table needs to be expanded */
			wasm::IfThen _if0{ sink };
			sink[I::Ref::NullFunction()];
			sink[I::U32::Const(glue::CoreMapping::_count)];
			sink[I::Table::Grow(state.coreFunctions)];

			/* check if the allocation failed and return 0 */
			sink[I::I32::Const(-1)];
			sink[I::U32::Equal()];
			wasm::IfThen _if1{ sink };
			sink[I::U32::Const(0)];
			sink[I::Return()];
		}

		/* try to grow the core-list (no need to check its size, as its
		*	the last allocation, and hence cannot yet have the slot) */
		sink[I::Ref::NullExtern()];
		sink[I::U32::Const(1)];
		sink[I::Table::Grow(state.cores)];

		/* check if the allocation failed and return 0 */
		sink[I::I32::Const(-1)];
		sink[I::U32::Equal()];
		{
			wasm::IfThen _if{ sink };
			sink[I::U32::Const(0)];
			sink[I::Return()];
		}

		/* update the slot-state */
		sink[I::Local::Get(slotAddress)];
		sink[I::U32::Const(glue::SlotState::awaitingCore)];
		sink[I::U32::Store8(state.memory, offsetof(glue::Slot, state))];

		/* write the process-value out */
		sink[I::Local::Get(slotAddress)];
		sink[I::Local::Get(sink.parameter(0))];
		sink[I::U64::Store(state.memory, offsetof(glue::Slot, process))];

		/* advance the overall slot-count by one */
		sink[I::Global::Get(state.slotCount)];
		sink[I::U32::Const(1)];
		sink[I::U32::Add()];
		sink[I::Global::Set(state.slotCount)];

		/* return the index (still contains the new slot-value) */
		sink[I::Local::Get(index)];
	}

	/* add the context-load-core function */
	if (true) {
		wasm::Prototype prototype = state.module.prototype(u8"ctx_load_core_type",
			{ { u8"id", wasm::Type::i32 }, { u8"data", wasm::Type::i32 }, { u8"size", wasm::Type::i32 } },
			{ wasm::Type::i32 }
		);
		wasm::Sink sink{ state.module.function(u8"ctx_load_core", prototype, wasm::Export{}) };
		wasm::Variable slotAddress = sink.local(wasm::Type::i32, u8"slot_address");

		/* compute the slot address */
		sink[I::Local::Get(sink.parameter(0))];
		sink[I::U32::Const(sizeof(glue::Slot))];
		sink[I::U32::Mul()];
		sink[I::U32::Const(state.addressOfList)];
		sink[I::U32::Add()];
		sink[I::Local::Tee(slotAddress)];

		/* check if the slot-state is valid */
		sink[I::U32::Load8(state.memory, offsetof(glue::Slot, state))];
		sink[I::U32::Const(glue::SlotState::awaitingCore)];
		sink[I::U32::NotEqual()];
		{
			/* core has already been set, return 0 to indicate failure */
			wasm::IfThen _if{ sink };
			sink[I::U32::Const(0)];
			sink[I::Return()];
		}

		/* update the state */
		sink[I::Local::Get(slotAddress)];
		sink[I::U32::Const(glue::SlotState::loadingCore)];
		sink[I::U32::Store8(state.memory, offsetof(glue::Slot, state))];

		/* call the host function to create the core */
		sink[I::Local::Get(sink.parameter(0))];
		sink[I::Local::Get(sink.parameter(1))];
		sink[I::Local::Get(sink.parameter(2))];
		sink[I::Call::Direct(state.hostLoadCore)];

		/* return 1 to indicate start-of-loading */
		sink[I::U32::Const(1)];
		sink[I::Return()];
	}

	/* add the context-load-block function */
	if (true) {
		wasm::Prototype prototype = state.module.prototype(u8"ctx_load_block_type",
			{ { u8"id", wasm::Type::i32 }, { u8"data", wasm::Type::i32 }, { u8"size", wasm::Type::i32 }, { u8"exports", wasm::Type::i32 } },
			{ wasm::Type::i32 }
		);
		wasm::Sink sink{ state.module.function(u8"ctx_load_block", prototype, wasm::Export{}) };
		wasm::Variable slotAddress = sink.local(wasm::Type::i32, u8"slot_address");

		/* compute the slot address */
		sink[I::Local::Get(sink.parameter(0))];
		sink[I::U32::Const(sizeof(glue::Slot))];
		sink[I::U32::Mul()];
		sink[I::U32::Const(state.addressOfList)];
		sink[I::U32::Add()];
		sink[I::Local::Tee(slotAddress)];

		/* check if the slot-state is correct for loading */
		sink[I::U32::Load8(state.memory, offsetof(glue::Slot, state))];
		sink[I::U32::Const(glue::SlotState::coreLoaded)];
		sink[I::U32::NotEqual()];
		{
			/* core has not been set or other loads are currently in progress, return 0 to indicate failure */
			wasm::IfThen _if{ sink };
			sink[I::U32::Const(0)];
			sink[I::Return()];
		}

		/* try to reserve the number of exports and the single block */
		sink[I::Local::Get(sink.parameter(3))];
		sink[I::Local::Get(sink.parameter(0))];
		sink[I::U32::Const(glue::CoreMapping::_count)];
		sink[I::U32::Mul()];
		sink[I::U32::Const(glue::CoreMapping::blocksReserve)];
		sink[I::U32::Add()];
		sink[I::Call::Indirect(state.coreFunctions, { wasm::Type::i32 }, { wasm::Type::i32 })];
		sink[I::U32::EqualZero()];
		{
			/* unable to reserve slots, return 0 to indicate failure */
			wasm::IfThen _if{ sink };
			sink[I::U32::Const(0)];
			sink[I::Return()];
		}

		/* update the state */
		sink[I::Local::Get(slotAddress)];
		sink[I::U32::Const(glue::SlotState::loadingBlock)];
		sink[I::U32::Store8(state.memory, offsetof(glue::Slot, state))];

		/* call the host function to create the block */
		sink[I::Local::Get(sink.parameter(0))];
		sink[I::Local::Get(sink.parameter(0))];
		sink[I::Table::Get(state.cores)];
		sink[I::Local::Get(sink.parameter(1))];
		sink[I::Local::Get(sink.parameter(2))];
		sink[I::Call::Direct(state.hostLoadBlock)];

		/* return 1 to indicate start-of-loading */
		sink[I::U32::Const(1)];
		sink[I::Return()];
	}

	/* add the context-destroy function */
	if (true) {
		wasm::Prototype prototype = state.module.prototype(u8"ctx_destroy_type",
			{ { u8"id", wasm::Type::i32 } },
			{}
		);
		wasm::Sink sink{ state.module.function(u8"ctx_destroy", prototype, wasm::Export{}) };
		wasm::Variable slotAddress = sink.local(wasm::Type::i32, u8"slot_address");

		/* compute the slot address */
		sink[I::Local::Get(sink.parameter(0))];
		sink[I::U32::Const(sizeof(glue::Slot))];
		sink[I::U32::Mul()];
		sink[I::U32::Const(state.addressOfList)];
		sink[I::U32::Add()];
		sink[I::Local::Tee(slotAddress)];

		/* check if the slot-state is valid */
		sink[I::U32::Load8(state.memory, offsetof(glue::Slot, state))];
		sink[I::U32::Const(glue::SlotState::reserved)];
		sink[I::U32::Equal()];
		{
			/* cannot be cleared, simply return */
			wasm::IfThen _if{ sink };
			sink[I::Return()];
		}

		/* clear the core-entry */
		sink[I::Local::Get(sink.parameter(0))];
		sink[I::Ref::NullExtern()];
		sink[I::Table::Set(state.cores)];

		/* clear the function-list (to ensure all references can be dropped) */
		sink[I::Local::Get(sink.parameter(0))];
		sink[I::U32::Const(glue::CoreMapping::_count)];
		sink[I::U32::Mul()];
		sink[I::Ref::NullFunction()];
		sink[I::U32::Const(glue::CoreMapping::_count)];
		sink[I::Table::Fill(state.coreFunctions)];

		/* update the slot-state */
		sink[I::Local::Get(slotAddress)];
		sink[I::U32::Const(glue::SlotState::available)];
		sink[I::U32::Store8(state.memory, offsetof(glue::Slot, state))];
	}

	/* add the add-export function */
	if (true) {
		wasm::Prototype prototype = state.module.prototype(u8"ctx_add_export_type",
			{ { u8"id", wasm::Type::i32 }, { u8"name", wasm::Type::i32 }, { u8"size", wasm::Type::i32 }, { u8"address", wasm::Type::i64 } },
			{}
		);
		wasm::Sink sink{ state.module.function(u8"ctx_add_export", prototype, wasm::Export{}) };
		wasm::Variable index = sink.local(wasm::Type::i32, u8"index");

		/* compute the index into the core-functions table */
		sink[I::Local::Get(sink.parameter(0))];
		sink[I::U32::Const(glue::CoreMapping::_count)];
		sink[I::U32::Mul()];
		sink[I::Local::Tee(index)];

		/* fetch the reference to the last loaded block */
		sink[I::U32::Const(glue::CoreMapping::blocksGetLast)];
		sink[I::U32::Add()];
		sink[I::Call::Indirect(state.coreFunctions, {}, { wasm::Type::refExtern })];

		/* resolve the exported function */
		sink[I::Local::Get(sink.parameter(1))];
		sink[I::Local::Get(sink.parameter(2))];
		sink[I::Call::Direct(state.hostGetBlockFunction)];

		/* write the function back to the core module */
		sink[I::Local::Get(sink.parameter(3))];
		sink[I::Local::Get(index)];
		sink[I::U32::Const(glue::CoreMapping::blocksAddExport)];
		sink[I::U32::Add()];
		sink[I::Call::Indirect(state.coreFunctions, { wasm::Type::refFunction, wasm::Type::i64 }, {})];
	}
}
