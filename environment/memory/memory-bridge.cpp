#include "../env-process.h"
#include "../../interface/interface.h"

void env::detail::MemoryBridge::Lookup(uint64_t address, uint32_t size, uint32_t usage) {
	env::Instance()->memory().pMapper.lookup(address, size, usage);
}
uint64_t env::detail::MemoryBridge::LookupAddress() {
	return env::Instance()->memory().pMapper.lastLookup().address;
}
uint32_t env::detail::MemoryBridge::LookupSize() {
	return env::Instance()->memory().pMapper.lastLookup().size;
}
uint32_t env::detail::MemoryBridge::LookupPhysical() {
	return env::Instance()->memory().pMapper.lastLookup().physical;
}

bool env::detail::MemoryBridge::ExpandPhysical(uint32_t pages) {
	return (mem_expand_physical(pages) == 0);
}
void env::detail::MemoryBridge::MovePhysical(env::physical_t dest, env::physical_t source, uint32_t size) {
	mem_move_physical(dest, source, size);
}
void env::detail::MemoryBridge::FlushCaches() {
	mem_flush_caches();
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
