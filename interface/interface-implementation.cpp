#include "interface.h"
#include "../glue/generate.h"
#include "../env/env-process.h"

#include <ustring/ustring.h>
#include <vector>
#include <fstream>

/* null-implementation for execution without glue-module */
#ifndef EMSCRIPTEN_COMPILATION

// #error currently not supported

int main() {
	//main_startup();
	//return 0;

	writer::TextWriter _core, _block;
	{
		wasm::Module _modCore{ &_core };
		wasm::Module _modBlock{ &_block };
		env::Process* proc = env::Process::Create(u8"test_module", 4, [](env::guest_t addr) {}, [](int32_t) {});
		if (proc) {
			proc->setupCoreModule(_modCore);
			proc->setupBlockModule(_modBlock);
			proc->release();
		}
	}
	const std::u8string& core = _core.output();
	const std::u8string& block = _block.output();

	std::fstream{ "source/server/generated/core-example.wat", std::ios::out }.write(reinterpret_cast<const char*>(core.data()), core.size());
	std::fstream{ "source/server/generated/block-example.wat", std::ios::out }.write(reinterpret_cast<const char*>(block.data()), block.size());
	return 0;
}

void host_print_u8(const char8_t* data, uint32_t size) {
	str::PrintLn(std::u8string_view{ data, size });
}
void host_fail_u8 [[noreturn]] (const char8_t* data, uint32_t size) {
	str::PrintLn(u8"Exception: ", std::u8string_view{ data, size });
	exit(1);
}

static std::vector<uint64_t> ProcessList = { 0 };
static uint64_t MemorySelf = 0;

uint32_t ctx_create(uint64_t process) {
	ProcessList.push_back(process);
	return uint32_t(ProcessList.size() - 1);
}
uint32_t ctx_load_core(uint32_t id, const uint8_t* data, uint32_t size) {
	return {};
}
uint32_t ctx_load_block(uint32_t id, const uint8_t* data, uint32_t size, uint32_t exports) {
	return {};
}
uint32_t ctx_set_core(uint32_t id, const uint8_t* data, uint32_t size) {
	ctx_core_loaded(ProcessList[id], 1);
	return 1;
}
void ctx_destroy(uint32_t id) {}
void ctx_add_export(uint32_t id, const char8_t* name, uint32_t size, uint64_t address) {}

void map_execute(uint32_t id, uint64_t address) {}
void map_flush_blocks(uint32_t id) {}

uint32_t mem_expand_physical(uint32_t id, uint32_t pages) { return {}; }
void mem_move_physical(uint32_t id, uint32_t dest, uint32_t source, uint32_t size) {}
void mem_flush_caches(uint32_t id) {}
uint32_t mem_read_u8_i32(uint32_t id, uint64_t address) {
	mem_lookup(MemorySelf, address, 1, env::MemoryUsage::Read);
	return {};
}
uint32_t mem_read_i8_i32(uint32_t id, uint64_t address) {
	mem_lookup(MemorySelf, address, 1, env::MemoryUsage::Read);
	return {};
}
uint32_t mem_read_u16_i32(uint32_t id, uint64_t address) {
	mem_lookup(MemorySelf, address, 2, env::MemoryUsage::Read);
	return {};
}
uint32_t mem_read_i16_i32(uint32_t id, uint64_t address) {
	mem_lookup(MemorySelf, address, 2, env::MemoryUsage::Read);
	return {};
}
uint32_t mem_read_i32(uint32_t id, uint64_t address) {
	mem_lookup(MemorySelf, address, 4, env::MemoryUsage::Read);
	return {};
}
uint64_t mem_read_i64(uint32_t id, uint64_t address) {
	mem_lookup(MemorySelf, address, 8, env::MemoryUsage::Read);
	return {};
}
float mem_read_f32(uint32_t id, uint64_t address) {
	mem_lookup(MemorySelf, address, 4, env::MemoryUsage::Read);
	return {};
}
double mem_read_f64(uint32_t id, uint64_t address) {
	mem_lookup(MemorySelf, address, 8, env::MemoryUsage::Read);
	return {};
}
void mem_write_u8_i32(uint32_t id, uint64_t address, uint32_t value) {
	mem_lookup(MemorySelf, address, 1, env::MemoryUsage::Write);
}
void mem_write_i8_i32(uint32_t id, uint64_t address, uint32_t value) {
	mem_lookup(MemorySelf, address, 1, env::MemoryUsage::Write);
}
void mem_write_u16_i32(uint32_t id, uint64_t address, uint32_t value) {
	mem_lookup(MemorySelf, address, 2, env::MemoryUsage::Write);
}
void mem_write_i16_i32(uint32_t id, uint64_t address, uint32_t value) {
	mem_lookup(MemorySelf, address, 2, env::MemoryUsage::Write);
}
void mem_write_i32(uint32_t id, uint64_t address, uint32_t value) {
	mem_lookup(MemorySelf, address, 4, env::MemoryUsage::Write);
}
void mem_write_i64(uint32_t id, uint64_t address, uint64_t value) {
	mem_lookup(MemorySelf, address, 8, env::MemoryUsage::Write);
}
void mem_write_f32(uint32_t id, uint64_t address, float value) {
	mem_lookup(MemorySelf, address, 4, env::MemoryUsage::Write);
}
void mem_write_f64(uint32_t id, uint64_t address, double value) {
	mem_lookup(MemorySelf, address, 8, env::MemoryUsage::Write);
}
uint32_t mem_execute_u8_i32(uint32_t id, uint64_t address) {
	mem_lookup(MemorySelf, address, 1, env::MemoryUsage::Execute);
	return {};
}
uint32_t mem_execute_i8_i32(uint32_t id, uint64_t address) {
	mem_lookup(MemorySelf, address, 1, env::MemoryUsage::Execute);
	return {};
}
uint32_t mem_execute_u16_i32(uint32_t id, uint64_t address) {
	mem_lookup(MemorySelf, address, 2, env::MemoryUsage::Execute);
	return {};
}
uint32_t mem_execute_i16_i32(uint32_t id, uint64_t address) {
	mem_lookup(MemorySelf, address, 2, env::MemoryUsage::Execute);
	return {};
}
uint32_t mem_execute_i32(uint32_t id, uint64_t address) {
	mem_lookup(MemorySelf, address, 4, env::MemoryUsage::Execute);
	return {};
}
uint64_t mem_execute_i64(uint32_t id, uint64_t address) {
	mem_lookup(MemorySelf, address, 8, env::MemoryUsage::Execute);
	return {};
}
float mem_execute_f32(uint32_t id, uint64_t address) {
	mem_lookup(MemorySelf, address, 4, env::MemoryUsage::Execute);
	return {};
}
double mem_execute_f64(uint32_t id, uint64_t address) {
	mem_lookup(MemorySelf, address, 8, env::MemoryUsage::Execute);
	return {};
}

#endif
