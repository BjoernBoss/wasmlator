#include "../environment.h"
#include "../../interface/interface.h"

void env::detail::MemoryBridge::Lookup(uint64_t address, uint64_t access, uint32_t size, uint32_t usage, uint32_t cache) {
	env::Instance()->memory().fCacheLookup(address, access, size, usage, cache);
}

bool env::detail::MemoryBridge::ExpandPhysical(uint32_t pages) {
	return (mem_expand_physical(pages) > 0);
}
void env::detail::MemoryBridge::MovePhysical(detail::physical_t dest, detail::physical_t source, uint32_t size) {
	mem_move_physical(dest, source, size);
}
void env::detail::MemoryBridge::WriteToPhysical(detail::physical_t dest, const uint8_t* source, uint32_t size) {
	mem_write_to_physical(dest, source, size);
}
void env::detail::MemoryBridge::ReadFromPhysical(uint8_t* dest, detail::physical_t source, uint32_t size) {
	mem_read_from_physical(dest, source, size);
}
void env::detail::MemoryBridge::ClearPhysical(detail::physical_t dest, uint32_t size) {
	mem_clear_physical(dest, size);
}
uint64_t env::detail::MemoryBridge::Read(env::guest_t address, uint32_t size) {
	return mem_read(address, size);
}
void env::detail::MemoryBridge::Write(env::guest_t address, uint32_t size, uint64_t value) {
	mem_write(address, size, value);
}
uint64_t env::detail::MemoryBridge::Code(env::guest_t address, uint32_t size) {
	return mem_code(address, size);
}
