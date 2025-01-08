#include "../host/interface.h"
#include "../environment/environment.h"

#include <ustring/ustring.h>
#include <vector>

uint32_t host_load_core(const uint8_t* data, uint32_t size, uint32_t process) {
	main_core_loaded(process);
	return 1;
}
uint32_t host_load_block(const uint8_t* data, uint32_t size, uint32_t process) {
	main_block_loaded(process);
	return 1;
}
void host_print_u8(const char8_t* data, uint32_t size, bool failure) {
	if (failure)
		str::BuildTo(std::cerr, u8"Fatal Exception: ", std::u8string_view{ data, size }, '\n');
	else
		str::BuildTo(std::cout, std::u8string_view{ data, size }, '\n');
}
uint32_t host_random() { return 123; }

uint32_t glue_setup_core_map() { return 1; }
void glue_reset_core_map() {}


uint32_t proc_export(const char8_t* name, uint32_t size, uint32_t index) { return 1; }
void proc_block_imports_prepare() {}
void proc_block_imports_next_member(const char8_t* name, uint32_t size) {}
void proc_block_imports_set_value(const char8_t* name, uint32_t size, uint32_t index) {}
void proc_block_imports_commit(uint32_t null) {}

uint32_t map_reserve(uint32_t exports) { return 1; }
uint32_t map_define(const char8_t* name, uint32_t size, uint64_t address) { return {}; }
uint64_t map_execute(uint64_t address) { return {}; }

static std::vector<uint8_t> memory = std::vector<uint8_t>(env::detail::PhysPageSize * env::detail::InitAllocPages);
void mem_write_to_physical(uint32_t dest, const void* source, uint32_t size) {
	const uint8_t* _source = static_cast<const uint8_t*>(source);
	std::copy(_source, _source + size, memory.begin() + dest);
}
void mem_read_from_physical(void* dest, uint32_t source, uint32_t size) {
	uint8_t* _dest = static_cast<uint8_t*>(dest);
	std::copy(memory.begin() + source, memory.begin() + source + size, _dest);
}
void mem_clear_physical(uint32_t dest, uint32_t size) {
	std::fill(memory.begin() + dest, memory.begin() + dest + size, 0);
}
uint32_t mem_expand_physical(uint32_t pages) {
	memory.resize(memory.size() + pages * env::detail::PhysPageSize);
	return 1;
}
void mem_move_physical(uint32_t dest, uint32_t source, uint32_t size) {
	std::memmove(memory.data() + dest, memory.data() + source, size);
}
uint64_t mem_read(uint64_t address, uint32_t size) {
	uint64_t buffer = 0;
	env::Instance()->memory().mread(reinterpret_cast<void*>(&buffer), address, size, env::Usage::Read);
	return buffer;
}
void mem_write(uint64_t address, uint32_t size, uint64_t value) {
	env::Instance()->memory().mwrite(address, reinterpret_cast<const void*>(&value), size, env::Usage::Write);
}
uint64_t mem_code(uint64_t address, uint32_t size) {
	uint64_t buffer = 0;
	env::Instance()->memory().mread(reinterpret_cast<void*>(&buffer), address, size, env::Usage::Execute);
	return buffer;
}

void int_call_void(uint32_t index) {}
uint64_t int_call_param(uint64_t param, uint32_t index) { return{}; }
