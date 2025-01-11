#pragma once

#include <cinttypes>
#include <string>

/* primary application entry point */
void HandleCommand(std::u8string_view commcmdand);

/* environment/entry-point interactions */
extern "C" {
	/* exports */
	void main_command(char8_t* ptr, uint32_t size);
	void* main_allocate(uint32_t size);

	/* helper */
	void free_allocated(void* ptr);
}

/* glue/host interactions */
extern "C" {
	/* imports */
	uint32_t host_load_core(const uint8_t* data, uint32_t size, uint32_t process);
	uint32_t host_load_block(const uint8_t* data, uint32_t size, uint32_t process);
	void host_print_u8(const char8_t* data, uint32_t size, bool failure);
	uint32_t host_random();

	uint32_t glue_setup_core_map();
	void glue_reset_core_map();
}

/* environment/process/process-bridge interactions */
extern "C" {
	/* exports */
	void main_core_loaded(uint32_t process);
	void main_block_loaded(uint32_t process);

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
	void main_terminate(int32_t code, uint64_t address);
	void main_code_exception(uint64_t address, uint32_t id);
}

/* environment/mapping/mapping-bridge interactions */
extern "C" {
	/* exports */
	uint32_t main_resolve(uint64_t address);

	/* imports */
	uint32_t map_reserve(uint32_t exports);
	uint32_t map_define(const char8_t* name, uint32_t size, uint64_t address);
	uint64_t map_execute(uint64_t address);
}

/* environment/memory/memory-bridge interactions */
extern "C" {
	/* exports */
	void main_lookup(uint64_t address, uint64_t access, uint32_t size, uint32_t usage, uint32_t cache);

	/* imports */
	void mem_write_to_physical(uint32_t dest, const void* source, uint32_t size);
	void mem_read_from_physical(void* dest, uint32_t source, uint32_t size);
	void mem_clear_physical(uint32_t dest, uint32_t size);
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
