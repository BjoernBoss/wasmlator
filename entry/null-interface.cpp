#include "../interface/interface.h"
#include "../environment/env-process.h"

#include <ustring/ustring.h>
#include <vector>

void host_print_u8(const char8_t* data, uint32_t size, uint32_t error) {
	str::BuildTo((error > 0 ? std::cerr : std::cout), std::u8string_view{ data, size }, '\n');
}
void host_abort [[noreturn]] () {
	exit(1);
}

uint32_t host_load_core(const uint8_t* data, uint32_t size, uint32_t process) {
	main_core_loaded(process, 1);
	return 1;
}
uint32_t proc_setup_core_functions() { return 1; }
void host_define_block_binding(const char8_t* modName, uint32_t modSize, const char8_t* name, uint32_t size) {}

uint64_t ctx_read(uint32_t offset, uint32_t size) { return{}; }
void ctx_write(uint32_t offset, uint32_t size, uint64_t value) {}

uint32_t map_load_block(const uint8_t* data, uint32_t size, uint32_t exports, uint32_t process) {
	main_block_loaded(process, 1);
	return 1;
}
uint32_t map_define(const char8_t* name, uint32_t size, uint64_t address) { return {}; }
uint32_t map_execute(uint64_t address) { return {}; }
void map_flush_blocks() {}

void mem_flush_caches() {}
uint32_t mem_expand_physical(uint32_t pages) { return{}; }
void mem_move_physical(uint32_t dest, uint32_t source, uint32_t size) {}
uint64_t mem_read(uint64_t address, uint32_t size) { return{}; }
void mem_write(uint64_t address, uint32_t size, uint64_t value) {}
uint64_t mem_code(uint64_t address, uint32_t size) { return{}; }

void int_call_void(uint32_t index) {}
uint64_t int_call_param(uint64_t param, uint32_t index) { return{}; }
