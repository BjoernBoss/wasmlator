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
	void host_fail_u8 [[noreturn]] (const char8_t* data, uint32_t size);
}

/* env/context/context-bridge interactions */
extern "C" {
	/* exports */
	void ctx_core_loaded(uint64_t process, uint32_t succeeded);
	void ctx_translate(uint64_t process, uint64_t address);

	/* imports */
	uint32_t ctx_create(uint64_t process);
	uint32_t ctx_set_core(uint32_t id, const uint8_t* data, uint32_t size);
	void ctx_destroy(uint32_t id);
}

/* env/blocks/blocks-bridge interactions */
extern "C" {
	/* exports */
	uint32_t blocks_lookup_complex(uint64_t process, uint64_t address);
	void blocks_flushed(uint64_t process);
	void blocks_associate(uint64_t process, uint64_t address, uint32_t index);

	/* imports */
	void blocks_execute(uint32_t id, uint64_t address);
	void blocks_flush_blocks(uint32_t id);
}

/* env/memory/memory-bridge interactions */
extern "C" {
	/* exports */
	uint64_t mem_perform_lookup(uint64_t process, uint64_t address, uint32_t size, uint32_t usage);
	uint32_t mem_result_physical(uint64_t process);
	uint32_t mem_result_size(uint64_t process);

	/* imports */
	uint32_t mem_expand_physical(uint32_t id, uint32_t pages);
	void mem_move_physical(uint32_t id, uint32_t dest, uint32_t source, uint32_t size);
	void mem_flush_caches(uint32_t id);
	uint32_t mem_read_u8_i32(uint32_t id, uint64_t address);
	uint32_t mem_read_i8_i32(uint32_t id, uint64_t address);
	uint32_t mem_read_u16_i32(uint32_t id, uint64_t address);
	uint32_t mem_read_i16_i32(uint32_t id, uint64_t address);
	uint32_t mem_read_i32(uint32_t id, uint64_t address);
	uint64_t mem_read_i64(uint32_t id, uint64_t address);
	float mem_read_f32(uint32_t id, uint64_t address);
	double mem_read_f64(uint32_t id, uint64_t address);
	void mem_write_u8_i32(uint32_t id, uint64_t address, uint32_t value);
	void mem_write_i8_i32(uint32_t id, uint64_t address, uint32_t value);
	void mem_write_u16_i32(uint32_t id, uint64_t address, uint32_t value);
	void mem_write_i16_i32(uint32_t id, uint64_t address, uint32_t value);
	void mem_write_i32(uint32_t id, uint64_t address, uint32_t value);
	void mem_write_i64(uint32_t id, uint64_t address, uint64_t value);
	void mem_write_f32(uint32_t id, uint64_t address, float value);
	void mem_write_f64(uint32_t id, uint64_t address, double value);
	uint32_t mem_execute_u8_i32(uint32_t id, uint64_t address);
	uint32_t mem_execute_i8_i32(uint32_t id, uint64_t address);
	uint32_t mem_execute_u16_i32(uint32_t id, uint64_t address);
	uint32_t mem_execute_i16_i32(uint32_t id, uint64_t address);
	uint32_t mem_execute_i32(uint32_t id, uint64_t address);
	uint64_t mem_execute_i64(uint32_t id, uint64_t address);
	float mem_execute_f32(uint32_t id, uint64_t address);
	double mem_execute_f64(uint32_t id, uint64_t address);
}
