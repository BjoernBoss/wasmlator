#include "generate.h"

static void WriteMainString(glue::State& state, glue::MainMapping index, std::u8string_view string) {
	/* write the string identifiers out */
	state.mainStrings[size_t(index)].offset = state.addressOfList;
	state.mainStrings[size_t(index)].length = uint32_t(string.size());

	/* write the string to the data section */
	state.module.data(state.memory, wasm::Value::MakeU32(state.addressOfList), reinterpret_cast<const uint8_t*>(string.data()), uint32_t(string.size()));
	state.addressOfList += uint32_t(string.size());
}
static void WriteCoreString(glue::State& state, glue::CoreMapping index, std::u8string_view string) {
	/* write the string identifiers out */
	state.coreStrings[size_t(index)].offset = state.addressOfList;
	state.coreStrings[size_t(index)].length = uint32_t(string.size());

	/* write the string to the data section */
	state.module.data(state.memory, wasm::Value::MakeU32(state.addressOfList), reinterpret_cast<const uint8_t*>(string.data()), uint32_t(string.size()));
	state.addressOfList += uint32_t(string.size());
}

void glue::InitializeState(glue::State& state) {
	/* add the main function table and loaded-flag */
	state.mainFunctions = state.module.table(u8"main_functions", true, wasm::Limit{ uint32_t(glue::MainMapping::_count), uint32_t(glue::MainMapping::_count) });
	state.mainLoaded = state.module.global(u8"main_loaded", wasm::Type::i32, true);
	state.module.value(state.mainLoaded, wasm::Value::MakeU32(glue::MainState::notLoaded));

	/* add the function table and the core-table */
	state.coreFunctions = state.module.table(u8"function_list", true, wasm::Limit{ glue::MinCoreCount * uint32_t(glue::CoreMapping::_count) });
	state.cores = state.module.table(u8"core_list", false, wasm::Limit{ glue::MinCoreCount });

	/* add the memory and global slot-count (initialize it with 1 to reserve the null-entry; assumption
	*	that 1 physical page is large enough to hold all strings and the first glue::Slot) */
	state.memory = state.module.memory(u8"memory", wasm::Limit{ 1 }, wasm::Export{});
	state.slotCount = state.module.global(u8"slot_count", wasm::Type::i32, true);
	state.module.value(state.slotCount, wasm::Value::MakeU32(1));

	/* write the strings out, which identify all main-mapping-functions */
	WriteMainString(state, glue::MainMapping::_initialize, u8"_initialize");
	WriteMainString(state, glue::MainMapping::mainStartup, u8"main_startup");
	WriteMainString(state, glue::MainMapping::contextCoreLoaded, u8"ctx_core_loaded");

	/* write the strings out, which identify all coremapping-functions */
	WriteCoreString(state, glue::CoreMapping::execute, u8"blocks_execute");
	WriteCoreString(state, glue::CoreMapping::flushBlocks, u8"blocks_flush_blocks");

	WriteCoreString(state, glue::CoreMapping::expandPhysical, u8"mem_expand_physical");
	WriteCoreString(state, glue::CoreMapping::movePhysical, u8"mem_move_physical");
	WriteCoreString(state, glue::CoreMapping::flushCaches, u8"mem_flush_caches");

	WriteCoreString(state, glue::CoreMapping::readi32Fromu8, u8"mem_read_u8_i32");
	WriteCoreString(state, glue::CoreMapping::readi32Fromi8, u8"mem_read_i8_i32");
	WriteCoreString(state, glue::CoreMapping::readi32Fromu16, u8"mem_read_u16_i32");
	WriteCoreString(state, glue::CoreMapping::readi32Fromi16, u8"mem_read_i16_i32");
	WriteCoreString(state, glue::CoreMapping::readi32, u8"mem_read_i32");
	WriteCoreString(state, glue::CoreMapping::readi64, u8"mem_read_i64");
	WriteCoreString(state, glue::CoreMapping::readf32, u8"mem_read_f32");
	WriteCoreString(state, glue::CoreMapping::readf64, u8"mem_read_f64");

	WriteCoreString(state, glue::CoreMapping::writei32Fromu8, u8"mem_write_u8_i32");
	WriteCoreString(state, glue::CoreMapping::writei32Fromi8, u8"mem_write_i8_i32");
	WriteCoreString(state, glue::CoreMapping::writei32Fromu16, u8"mem_write_u16_i32");
	WriteCoreString(state, glue::CoreMapping::writei32Fromi16, u8"mem_write_i16_i32");
	WriteCoreString(state, glue::CoreMapping::writei32, u8"mem_write_i32");
	WriteCoreString(state, glue::CoreMapping::writei64, u8"mem_write_i64");
	WriteCoreString(state, glue::CoreMapping::writef32, u8"mem_write_f32");
	WriteCoreString(state, glue::CoreMapping::writef64, u8"mem_write_f64");

	WriteCoreString(state, glue::CoreMapping::executei32Fromu8, u8"mem_execute_u8_i32");
	WriteCoreString(state, glue::CoreMapping::executei32Fromi8, u8"mem_execute_i8_i32");
	WriteCoreString(state, glue::CoreMapping::executei32Fromu16, u8"mem_execute_u16_i32");
	WriteCoreString(state, glue::CoreMapping::executei32Fromi16, u8"mem_execute_i16_i32");
	WriteCoreString(state, glue::CoreMapping::executei32, u8"mem_execute_i32");
	WriteCoreString(state, glue::CoreMapping::executei64, u8"mem_execute_i64");
	WriteCoreString(state, glue::CoreMapping::executef32, u8"mem_execute_f32");
	WriteCoreString(state, glue::CoreMapping::executef64, u8"mem_execute_f64");
}
