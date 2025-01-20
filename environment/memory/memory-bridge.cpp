#include "../environment.h"
#include "../../host/interface.h"

void env::detail::MemoryBridge::Lookup(uint64_t address, uint64_t access, uint32_t size, uint32_t usage, uint32_t cache) {
	env::Instance()->memory().fCacheLookup(address, access, size, usage, cache);
}

bool env::detail::MemoryBridge::ExpandPhysical(uint64_t pages) {
	if (pages >= detail::PhysMaxPages)
		return false;
	return (mem_expand_physical(uint32_t(pages)) > 0);
}
void env::detail::MemoryBridge::MovePhysical(uint64_t dest, uint64_t source, uint64_t size) {
	mem_move_physical(uint32_t(dest), uint32_t(source), uint32_t(size));
}
void env::detail::MemoryBridge::WriteToPhysical(uint64_t dest, const void* source, uint64_t size) {
	mem_write_to_physical(uint32_t(dest), source, uint32_t(size));
}
void env::detail::MemoryBridge::ReadFromPhysical(void* dest, uint64_t source, uint64_t size) {
	mem_read_from_physical(dest, uint32_t(source), uint32_t(size));
}
void env::detail::MemoryBridge::ClearPhysical(uint64_t dest, uint64_t size) {
	mem_clear_physical(uint32_t(dest), uint32_t(size));
}
uint64_t env::detail::MemoryBridge::Read(env::guest_t address, uint64_t size) {
	return mem_read(address, uint32_t(size));
}
void env::detail::MemoryBridge::Write(env::guest_t address, uint64_t size, uint64_t value) {
	mem_write(address, uint32_t(size), value);
}
uint64_t env::detail::MemoryBridge::Code(env::guest_t address, uint64_t size) {
	return mem_code(address, uint32_t(size));
}
