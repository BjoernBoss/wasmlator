#pragma once

#include <cinttypes>

/* env/entry-point interactions */
extern "C" {
	/* exports */
	void main_startup();
}

/* util/logging interactions */
extern "C" {
	/* imports */
	void host_print_u8(const char8_t* data, uint32_t size);
	void host_abort [[noreturn]] ();
}

/* env/process/process-bridge interactions */
extern "C" {
	/* exports */
	void proc_core_loaded(uint32_t succeeded);
	void proc_block_loaded(uint32_t succeeded);

	/* imports */
	uint32_t proc_create();
	void proc_destroy();
	uint32_t proc_load_core(const uint8_t* data, uint32_t size, uint32_t exports);
	uint32_t proc_load_block(const uint8_t* data, uint32_t size, uint32_t exports);
	void proc_add_core_export(const char8_t* name, uint32_t size, uint64_t address);
	void proc_add_block_export(const char8_t* name, uint32_t size, uint64_t address);
}

/* env/context/context-bridge interactions */
extern "C" {
	/* exports */
	void ctx_set_exit_code(int32_t code);

	/* imports */
	void ctx_read(uint32_t offset, uint8_t* data, uint32_t size);
	void ctx_write(uint32_t offset, const uint8_t* data, uint32_t size);
}

/* env/mapping/mapping-bridge interactions */
extern "C" {
	/* exports */
	uint32_t map_resolve(uint64_t address);
	void map_flushed();
	void map_associate(uint64_t address, uint32_t index);

	/* imports */
	uint32_t map_execute(uint64_t address);
	void map_flush_blocks();
}

/* env/memory/memory-bridge interactions */
extern "C" {
	/* exports */
	void mem_lookup(uint64_t address, uint32_t size, uint32_t usage);
	uint64_t mem_result_address();
	uint32_t mem_result_physical();
	uint32_t mem_result_size();

	/* imports */
	uint32_t mem_expand_physical(uint32_t pages);
	void mem_move_physical(uint32_t dest, uint32_t source, uint32_t size);
	void mem_flush_caches();
	uint32_t mem_read_u8_i32(uint64_t address);
	uint32_t mem_read_i8_i32(uint64_t address);
	uint32_t mem_read_u16_i32(uint64_t address);
	uint32_t mem_read_i16_i32(uint64_t address);
	uint32_t mem_read_i32(uint64_t address);
	uint64_t mem_read_i64(uint64_t address);
	float mem_read_f32(uint64_t address);
	double mem_read_f64(uint64_t address);
	void mem_write_u8_i32(uint64_t address, uint32_t value);
	void mem_write_i8_i32(uint64_t address, uint32_t value);
	void mem_write_u16_i32(uint64_t address, uint32_t value);
	void mem_write_i16_i32(uint64_t address, uint32_t value);
	void mem_write_i32(uint64_t address, uint32_t value);
	void mem_write_i64(uint64_t address, uint64_t value);
	void mem_write_f32(uint64_t address, float value);
	void mem_write_f64(uint64_t address, double value);
	uint32_t mem_code_u8_i32(uint64_t address);
	uint32_t mem_code_i8_i32(uint64_t address);
	uint32_t mem_code_u16_i32(uint64_t address);
	uint32_t mem_code_i16_i32(uint64_t address);
	uint32_t mem_code_i32(uint64_t address);
	uint64_t mem_code_i64(uint64_t address);
	float mem_code_f32(uint64_t address);
	double mem_code_f64(uint64_t address);
}
