#include "../interface/interface.h"
#include "../environment/env-process.h"

#include <ustring/ustring.h>
#include <vector>

uint32_t host_load_core(const uint8_t* data, uint32_t size, uint32_t process) {
	main_core_loaded(process, 1);
	return 1;
}
uint32_t host_load_block(const uint8_t* data, uint32_t size, uint32_t process) {
	main_block_loaded(process, 1);
	return 1;
}
void host_print_u8(const char8_t* data, uint32_t size) {
	str::BuildTo(std::cout, std::u8string_view{ data, size }, '\n');
}
void host_fatal_u8 [[noreturn]] (const char8_t* data, uint32_t size) {
	str::BuildTo(std::cerr, std::u8string_view{ data, size }, '\n');
	exit(1);
}
uint32_t glue_setup_core_map() { return 1; }
void glue_reset_core_map() {}


uint32_t proc_export(const char8_t* name, uint32_t size, uint32_t index) { return 1; }
void proc_block_imports_prepare() {}
void proc_block_imports_next_member(const char8_t* name, uint32_t size) {}
void proc_block_imports_set_value(const char8_t* name, uint32_t size, uint32_t index) {}
void proc_block_imports_commit(uint32_t null) {}

uint64_t ctx_read(uint32_t offset, uint32_t size) { return{}; }
void ctx_write(uint32_t offset, uint32_t size, uint64_t value) {}

uint32_t map_reserve(uint32_t exports) { return 1; }
uint32_t map_define(const char8_t* name, uint32_t size, uint64_t address) { return {}; }
void map_execute(uint64_t address) {}
void map_flush_blocks() {}

void mem_flush_caches() {}
uint32_t mem_expand_physical(uint32_t pages) { return{}; }
void mem_move_physical(uint32_t dest, uint32_t source, uint32_t size) {}
uint64_t mem_read(uint64_t address, uint32_t size) { return{}; }
void mem_write(uint64_t address, uint32_t size, uint64_t value) {}
uint64_t mem_code(uint64_t address, uint32_t size) { return{}; }

void int_call_void(uint32_t index) {}
uint64_t int_call_param(uint64_t param, uint32_t index) { return{}; }
