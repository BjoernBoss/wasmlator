#include "../interface/interface.h"
#include "../env/env-process.h"

#include <ustring/ustring.h>
#include <vector>

void host_print_u8(const char8_t* data, uint32_t size) {
	str::PrintLn(std::u8string_view{ data, size });
}
void host_abort [[noreturn]] () {
	exit(1);
}

static bool IsCreated = false;

uint32_t proc_create() {
	if (IsCreated)
		return 0;
	IsCreated = true;
	return 1;
}
uint32_t proc_load_core(const uint8_t* data, uint32_t size) {
	proc_core_loaded(1);
	return 1;
}
uint32_t proc_load_block(const uint8_t* data, uint32_t size, uint32_t exports) {
	return {};
}
void proc_destroy() {
	IsCreated = false;
}
void proc_add_export(const char8_t* name, uint32_t size, uint64_t address) {}

void ctx_read(uint32_t offset, uint8_t* data, uint32_t size) {}
void ctx_write(uint32_t offset, const uint8_t* data, uint32_t size) {}

uint32_t map_execute(uint64_t address) { return {}; }
void map_flush_blocks() {}

uint32_t mem_expand_physical(uint32_t pages) { return {}; }
void mem_move_physical(uint32_t dest, uint32_t source, uint32_t size) {}
void mem_flush_caches() {}
uint32_t mem_read_u8_i32(uint64_t address) {
	mem_lookup(address, 1, env::MemoryUsage::Read);
	return {};
}
uint32_t mem_read_i8_i32(uint64_t address) {
	mem_lookup(address, 1, env::MemoryUsage::Read);
	return {};
}
uint32_t mem_read_u16_i32(uint64_t address) {
	mem_lookup(address, 2, env::MemoryUsage::Read);
	return {};
}
uint32_t mem_read_i16_i32(uint64_t address) {
	mem_lookup(address, 2, env::MemoryUsage::Read);
	return {};
}
uint32_t mem_read_i32(uint64_t address) {
	mem_lookup(address, 4, env::MemoryUsage::Read);
	return {};
}
uint64_t mem_read_i64(uint64_t address) {
	mem_lookup(address, 8, env::MemoryUsage::Read);
	return {};
}
float mem_read_f32(uint64_t address) {
	mem_lookup(address, 4, env::MemoryUsage::Read);
	return {};
}
double mem_read_f64(uint64_t address) {
	mem_lookup(address, 8, env::MemoryUsage::Read);
	return {};
}
void mem_write_u8_i32(uint64_t address, uint32_t value) {
	mem_lookup(address, 1, env::MemoryUsage::Write);
}
void mem_write_i8_i32(uint64_t address, uint32_t value) {
	mem_lookup(address, 1, env::MemoryUsage::Write);
}
void mem_write_u16_i32(uint64_t address, uint32_t value) {
	mem_lookup(address, 2, env::MemoryUsage::Write);
}
void mem_write_i16_i32(uint64_t address, uint32_t value) {
	mem_lookup(address, 2, env::MemoryUsage::Write);
}
void mem_write_i32(uint64_t address, uint32_t value) {
	mem_lookup(address, 4, env::MemoryUsage::Write);
}
void mem_write_i64(uint64_t address, uint64_t value) {
	mem_lookup(address, 8, env::MemoryUsage::Write);
}
void mem_write_f32(uint64_t address, float value) {
	mem_lookup(address, 4, env::MemoryUsage::Write);
}
void mem_write_f64(uint64_t address, double value) {
	mem_lookup(address, 8, env::MemoryUsage::Write);
}
uint32_t mem_code_u8_i32(uint64_t address) {
	mem_lookup(address, 1, env::MemoryUsage::Execute);
	return {};
}
uint32_t mem_code_i8_i32(uint64_t address) {
	mem_lookup(address, 1, env::MemoryUsage::Execute);
	return {};
}
uint32_t mem_code_u16_i32(uint64_t address) {
	mem_lookup(address, 2, env::MemoryUsage::Execute);
	return {};
}
uint32_t mem_code_i16_i32(uint64_t address) {
	mem_lookup(address, 2, env::MemoryUsage::Execute);
	return {};
}
uint32_t mem_code_i32(uint64_t address) {
	mem_lookup(address, 4, env::MemoryUsage::Execute);
	return {};
}
uint64_t mem_code_i64(uint64_t address) {
	mem_lookup(address, 8, env::MemoryUsage::Execute);
	return {};
}
float mem_code_f32(uint64_t address) {
	mem_lookup(address, 4, env::MemoryUsage::Execute);
	return {};
}
double mem_code_f64(uint64_t address) {
	mem_lookup(address, 8, env::MemoryUsage::Execute);
	return {};
}
