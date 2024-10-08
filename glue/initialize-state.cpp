#include "generate.h"

static void WriteString(glue::State& state, glue::Mapping index, std::u8string_view string) {
	/* write the string identifiers out */
	state.strings[size_t(index)].offset = state.addressOfList;
	state.strings[size_t(index)].length = uint32_t(string.size());

	/* write the string to the data section */
	state.module.data(state.memory, wasm::Value::MakeU32(state.addressOfList), reinterpret_cast<const uint8_t*>(string.data()), uint32_t(string.size()));
	state.addressOfList += uint32_t(string.size());
}

void glue::InitializeState(glue::State& state) {
	/* add the function table and the core-table */
	state.functions = state.module.table(u8"function_list", true, wasm::Limit{ 2 * uint32_t(glue::Mapping::_count) });
	state.cores = state.module.table(u8"core_list", false, wasm::Limit{ 2 });

	/* add the memory and global slot-count (initialize it with 1 to reserve the null-entry) */
	state.memory = state.module.memory(u8"primary_memory", wasm::Limit{ 1 });
	state.slotCount = state.module.global(u8"slot_count", wasm::Type::i32, true);
	state.module.value(state.slotCount, wasm::Value::MakeU32(1));

	/* write the strings out, which identify all mapping-functions */
	WriteString(state, glue::Mapping::expandPhysical, u8"expand_physical");
	WriteString(state, glue::Mapping::movePhysical, u8"move_physical");
	WriteString(state, glue::Mapping::flushCaches, u8"flush_caches");

	WriteString(state, glue::Mapping::readi32Fromu8, u8"mem_read_i32_u8");
	WriteString(state, glue::Mapping::readi32Fromi8, u8"mem_read_i32_i8");
	WriteString(state, glue::Mapping::readi32Fromu16, u8"mem_read_i32_u16");
	WriteString(state, glue::Mapping::readi32Fromi16, u8"mem_read_i32_i16");
	WriteString(state, glue::Mapping::readi32, u8"mem_read_i32");
	WriteString(state, glue::Mapping::readi64, u8"mem_read_i64");
	WriteString(state, glue::Mapping::readf32, u8"mem_read_f32");
	WriteString(state, glue::Mapping::readf64, u8"mem_read_f64");

	WriteString(state, glue::Mapping::writei32Fromu8, u8"mem_write_i32_u8");
	WriteString(state, glue::Mapping::writei32Fromi8, u8"mem_write_i32_i8");
	WriteString(state, glue::Mapping::writei32Fromu16, u8"mem_write_i32_u16");
	WriteString(state, glue::Mapping::writei32Fromi16, u8"mem_write_i32_i16");
	WriteString(state, glue::Mapping::writei32, u8"mem_write_i32");
	WriteString(state, glue::Mapping::writei64, u8"mem_write_i64");
	WriteString(state, glue::Mapping::writef32, u8"mem_write_f32");
	WriteString(state, glue::Mapping::writef64, u8"mem_write_f64");

	WriteString(state, glue::Mapping::executei32Fromu8, u8"mem_execute_i32_u8");
	WriteString(state, glue::Mapping::executei32Fromi8, u8"mem_execute_i32_i8");
	WriteString(state, glue::Mapping::executei32Fromu16, u8"mem_execute_i32_u16");
	WriteString(state, glue::Mapping::executei32Fromi16, u8"mem_execute_i32_i16");
	WriteString(state, glue::Mapping::executei32, u8"mem_execute_i32");
	WriteString(state, glue::Mapping::executei64, u8"mem_execute_i64");
	WriteString(state, glue::Mapping::executef32, u8"mem_execute_f32");
	WriteString(state, glue::Mapping::executef64, u8"mem_execute_f64");
}
