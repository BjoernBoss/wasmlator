#include "generate.h"

namespace I = wasm::inst;

void glue::SetupContextFunctions(glue::State& state) {
	/* add the context-create function */
	{
		wasm::Sink sink = { state.module.function(u8"ctx_create", {}, { wasm::Type::i32 }, wasm::Export{}) };
		wasm::Variable index = sink.local(wasm::Type::i32, u8"index");

		/* iterate over the slots and look for the first '0' entry */
		{
			wasm::Block _exit{ sink, u8"exit_empty_slot_loop" };
			wasm::Loop _loop{ sink, u8"empty_slot_loop" };

			/* check if the end of the loop has been found */
			sink[I::Local::Get(index)];
			sink[I::Global::Get(state.slotCount)];
			sink[I::U32::GreaterEqual()];
			sink[I::Branch::If(_exit)];

			/* compute the offset into memory and load the value */
			sink[I::Local::Get(index)];
			sink[I::U32::Load8(state.memory, state.addressOfList)];

			/* check if an index has been found */
			sink[I::U32::Const(glue::SlotState::available)];
			sink[I::U32::NotEqual()];
			{
				wasm::IfThen _if{ sink };

				/* advance the index */
				sink[I::Local::Get(index)];
				sink[I::U32::Const(1)];
				sink[I::U32::Add()];
				sink[I::Local::Set(index)];

				/* go back to the loop header */
				sink[I::Branch::Direct(_loop)];
			}

			/* mark the index as allocated */
			sink[I::Local::Get(index)];
			sink[I::U32::Const(glue::SlotState::awaitingCore)];
			sink[I::U32::Store8(state.memory, state.addressOfList)];

			/* return the index (no need to alloate core/function slots as this is a
			*	reused context-index, hence the allocations have already occurred) */
			sink[I::Local::Get(index)];
			sink[I::Return()];
		}

		/* no entry has been found, try to expand the usage-list by one byte */
		sink[I::Global::Get(state.slotCount)];
		sink[I::U32::Const(state.addressOfList)];
		sink[I::U32::Add()];
		sink[I::Memory::Size(state.memory)];
		sink[I::U32::Const(glue::PageSize)];
		sink[I::U32::Mul()];
		sink[I::U32::GreaterEqual()];
		{
			/* memory needs to be expanded */
			wasm::IfThen _if0{ sink };
			sink[I::U32::Const(1)];
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
		sink[I::U32::Const(glue::Mapping::_count)];
		sink[I::U32::Mul()];
		sink[I::Table::Size(state.functions)];
		sink[I::U32::GreaterEqual()];
		{
			/* table needs to be expanded */
			wasm::IfThen _if0{ sink };
			sink[I::Ref::NullFunction()];
			sink[I::U32::Const(glue::Mapping::_count)];
			sink[I::Table::Grow(state.functions)];

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
		sink[I::Global::Get(state.slotCount)];
		sink[I::U32::Const(glue::SlotState::awaitingCore)];
		sink[I::U32::Store8(state.memory, state.addressOfList)];

		/* advance the overall slot-count by one */
		sink[I::Global::Get(state.slotCount)];
		sink[I::U32::Const(1)];
		sink[I::U32::Add()];
		sink[I::Global::Set(state.slotCount)];

		/* return the index (still contains the new slot-value) */
		sink[I::Local::Get(index)];
	}

	/* add the context-set-core function */
	{
		wasm::Sink sink{ state.module.function(u8"ctx_set_core", { wasm::Type::i32, wasm::Type::i32, wasm::Type::i32 }, { wasm::Type::i32 }, wasm::Export{}) };

		/* check if the slot-state is valid */
		sink[I::Local::Get(sink.parameter(0))];
		sink[I::U32::Load8(state.memory, state.addressOfList)];
		sink[I::U32::Const(glue::SlotState::awaitingCore)];
		sink[I::U32::NotEqual()];
		{
			/* core has already been set, return 0 to indicate failure */
			wasm::IfThen _if{ sink };
			sink[I::U32::Const(0)];
			sink[I::Return()];
		}

		/* update the state */
		sink[I::Local::Get(sink.parameter(0))];
		sink[I::U32::Const(glue::SlotState::loadingCore)];
		sink[I::U32::Store8(state.memory, state.addressOfList)];

		/* call the host function to create the core */
		sink[I::Local::Get(sink.parameter(0))];
		sink[I::Local::Get(sink.parameter(1))];
		sink[I::Local::Get(sink.parameter(2))];
		sink[I::Call::Direct(state.hostLoadCore)];

		/* return 1 to indicate success */
		sink[I::U32::Const(1)];
	}

	/* add the context-destroy function */
	{
		wasm::Sink sink{ state.module.function(u8"ctx_destroy", { wasm::Type::i32 }, {}, wasm::Export{}) };

		/* check if the slot-state is valid */
		sink[I::Local::Get(sink.parameter(0))];
		sink[I::U32::Load8(state.memory, state.addressOfList)];
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

		/* clear the function-list */
		sink[I::Local::Get(sink.parameter(0))];
		sink[I::U32::Const(glue::Mapping::_count)];
		sink[I::U32::Mul()];
		sink[I::Ref::NullFunction()];
		sink[I::U32::Const(glue::Mapping::_count)];
		sink[I::Table::Fill(state.functions)];

		/* update the slot-state */
		sink[I::Local::Get(sink.parameter(0))];
		sink[I::U32::Const(glue::SlotState::available)];
		sink[I::U32::Store8(state.memory, state.addressOfList)];
	}

	/* add the context-core-loaded function */
	{
		wasm::Sink sink{ state.module.function(u8"ctx_core_loaded", { wasm::Type::i32 }, { wasm::Type::i32 }, wasm::Export{}) };

		/* write the two select-options to the stack */
		sink[I::U32::Const(1)];
		sink[I::U32::Const(0)];

		/* check if the slot-state is valid */
		sink[I::Local::Get(sink.parameter(0))];
		sink[I::U32::Load8(state.memory, state.addressOfList)];
		sink[I::U32::Const(glue::SlotState::coreLoaded)];
		sink[I::U32::GreaterEqual()];

		/* select the corresponding result */
		sink[I::Select()];
	}
}
