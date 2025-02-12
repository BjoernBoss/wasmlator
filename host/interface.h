#pragma once

#include <cinttypes>
#include <string>

/* primary application entry point */
void HandleCommand(std::u8string_view cmd);

/* environment/host interactions */
extern "C" {
	/* exports */
	void main_user_command(const char8_t* ptr, uint32_t size);
	void main_task_completed(uint32_t process, const char8_t* ptr, uint32_t size);
	void* main_allocate(uint32_t size);

	/* imports */
	void host_task(const char8_t* task, uint32_t size, uint32_t process);
	void host_message(const char8_t* data, uint32_t size);
	void host_failure(const char8_t* data, uint32_t size);
	void host_guest_out(const char8_t* data, uint32_t size);
	uint32_t host_random();
	uint64_t host_time_us();
	int32_t host_timezone_min();
}

/* environment/process/process-bridge interactions */
extern "C" {
	/* imports */
	uint32_t glue_setup_core_map();
	void glue_reset_core_map();
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
	void map_execute(uint64_t address);
	void map_flush();
}

/* environment/memory/memory-bridge interactions */
extern "C" {
	/* exports */
	void main_check_lookup(uint64_t address, uint64_t access, uint32_t size, uint32_t usage, uint32_t cache);
	void main_fast_lookup(uint64_t access, uint32_t usage, uint32_t cache);
	void main_check_invalidated(uint64_t address);

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
