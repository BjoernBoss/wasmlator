#pragma once

#include <cinttypes>

/* environment/entry-point interactions */
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

/* environment/process/process-bridge interactions */
extern "C" {
	/* exports */
	void main_core_loaded(uint32_t succeeded);

	/* imports */
	uint32_t host_load_core(const uint8_t* data, uint32_t size);
	uint32_t proc_setup_core_functions();
}

/* environment/context/context-bridge interactions */
extern "C" {
	/* exports */
	void main_set_exit_code(int32_t code);

	/* imports */
	uint64_t ctx_read(uint32_t offset, uint32_t size);
	void ctx_write(uint32_t offset, uint32_t size, uint64_t value);
}

/* environment/mapping/mapping-bridge interactions */
extern "C" {
	/* exports */
	uint32_t main_resolve(uint64_t address);
	void main_flushed();
	void main_block_loaded(uint32_t succeeded);

	/* imports */
	uint32_t map_load_block(const uint8_t* data, uint32_t size, uint32_t exports);
	uint32_t map_define(const char8_t* name, uint32_t size, uint64_t address);
	void map_flush_blocks();
	uint32_t map_execute(uint64_t address);
}

/* environment/memory/memory-bridge interactions */
extern "C" {
	/* exports */
	void main_lookup(uint64_t address, uint32_t size, uint32_t usage);
	uint64_t main_result_address();
	uint32_t main_result_physical();
	uint32_t main_result_size();

	/* imports */
	void mem_flush_caches();
	uint32_t mem_expand_physical(uint32_t pages);
	void mem_move_physical(uint32_t dest, uint32_t source, uint32_t size);
	uint64_t mem_read(uint64_t address, uint32_t size);
	void mem_write(uint64_t address, uint32_t size, uint64_t value);
	uint64_t mem_code(uint64_t address, uint32_t size);
}
