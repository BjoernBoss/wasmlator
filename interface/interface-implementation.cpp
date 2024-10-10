#include "interface.h"
#include "../glue/generate.h"

#include <ustring/ustring.h>

/* null-implementation for execution without glue-module */
#ifndef EMSCRIPTEN_COMPILATION

// #error currently not supported

int main() {
	main_startup();
	return 0;
}

void host_print_u8(const char8_t* data, uint32_t size) {
	str::PrintLn(std::u8string_view{ data, size });
}
void host_fail_u8 [[noreturn]] (const char8_t* data, uint32_t size) {
	str::PrintLn(u8"Exception: ", std::u8string_view{ data, size });
	exit(1);
}

uint32_t ctx_create(uint64_t self) { return 1; }
uint32_t ctx_set_core(uint32_t id, const uint8_t* data, uint32_t size) { return 1; }
void ctx_destroy(uint32_t id) {}

uint32_t mem_expand_physical(uint32_t id, uint32_t pages) { return {}; }
void mem_move_physical(uint32_t id, uint32_t dest, uint32_t source, uint32_t size) {}
void mem_flush_caches(uint32_t id) {}
uint32_t mem_read_u8_i32(uint32_t id, uint64_t address) { return {}; }
uint32_t mem_read_i8_i32(uint32_t id, uint64_t address) { return {}; }
uint32_t mem_read_u16_i32(uint32_t id, uint64_t address) { return {}; }
uint32_t mem_read_i16_i32(uint32_t id, uint64_t address) { return {}; }
uint32_t mem_read_i32(uint32_t id, uint64_t address) { return {}; }
uint64_t mem_read_i64(uint32_t id, uint64_t address) { return {}; }
float mem_read_f32(uint32_t id, uint64_t address) { return {}; }
double mem_read_f64(uint32_t id, uint64_t address) { return {}; }
void mem_write_u8_i32(uint32_t id, uint64_t address, uint32_t value) {}
void mem_write_i8_i32(uint32_t id, uint64_t address, uint32_t value) {}
void mem_write_u16_i32(uint32_t id, uint64_t address, uint32_t value) {}
void mem_write_i16_i32(uint32_t id, uint64_t address, uint32_t value) {}
void mem_write_i32(uint32_t id, uint64_t address, uint32_t value) {}
void mem_write_i64(uint32_t id, uint64_t address, uint64_t value) {}
void mem_write_f32(uint32_t id, uint64_t address, float value) {}
void mem_write_f64(uint32_t id, uint64_t address, double value) {}
uint32_t mem_execute_u8_i32(uint32_t id, uint64_t address) { return {}; }
uint32_t mem_execute_i8_i32(uint32_t id, uint64_t address) { return {}; }
uint32_t mem_execute_u16_i32(uint32_t id, uint64_t address) { return {}; }
uint32_t mem_execute_i16_i32(uint32_t id, uint64_t address) { return {}; }
uint32_t mem_execute_i32(uint32_t id, uint64_t address) { return {}; }
uint64_t mem_execute_i64(uint32_t id, uint64_t address) { return {}; }
float mem_execute_f32(uint32_t id, uint64_t address) { return {}; }
double mem_execute_f64(uint32_t id, uint64_t address) { return {}; }

#endif
