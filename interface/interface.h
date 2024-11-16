#pragma once

#include <cinttypes>

/* environment/entry-point interactions */
extern "C" {
	/* exports */
	void main_startup();
}

/* glue/host interactions */
extern "C" {
	/* imports */
	uint32_t host_load_core(const uint8_t* data, uint32_t size, uint32_t process);
	uint32_t host_load_block(const uint8_t* data, uint32_t size, uint32_t process);
	void host_print_u8(const char8_t* data, uint32_t size, uint32_t error);
	void host_abort [[noreturn]] ();
	uint32_t glue_setup_core_map();
	void glue_reset_core_map();
}

/* environment/process/process-bridge interactions */
extern "C" {
	/* exports */
	void main_core_loaded(uint32_t process, uint32_t succeeded);
	void main_block_loaded(uint32_t process, uint32_t succeeded);

	/* imports */
	uint32_t proc_export(const char8_t* name, uint32_t size, uint32_t index);
	void proc_block_imports_prepare();
	void proc_block_imports_next_member(const char8_t* name, uint32_t size);
	void proc_block_imports_set_value(const char8_t* name, uint32_t size, uint32_t index);
	void proc_block_imports_commit(uint32_t null);
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

	/* imports */
	uint32_t map_reserve(uint32_t exports);
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

/* environment/interact/interact-bridge interactions */
extern "C" {
	/* exports */
	void main_invoke_void(uint32_t index);
	uint64_t main_invoke_param(uint64_t param, uint32_t index);

	/* imports */
	void int_call_void(uint32_t index);
	uint64_t int_call_param(uint64_t param, uint32_t index);
}
