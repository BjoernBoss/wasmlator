#include "../env-process.h"

uint64_t env::Memory::fRead(env::guest_t address, uint32_t size) const {
	return detail::MemoryBridge::Read(address, size);
}
void env::Memory::fWrite(env::guest_t address, uint32_t size, uint64_t value) const {
	detail::MemoryBridge::Write(address, size, value);
}
uint64_t env::Memory::fCode(env::guest_t address, uint32_t size) const {
	return detail::MemoryBridge::Code(address, size);
}

bool env::Memory::mmap(env::guest_t address, uint32_t size, uint32_t usage) {
	return pMapper.mmap(address, size, usage);
}
void env::Memory::munmap(env::guest_t address, uint32_t size) {
	return pMapper.munmap(address, size);
}
void env::Memory::mprotect(env::guest_t address, uint32_t size, uint32_t usage) {
	return pMapper.mprotect(address, size, usage);
}
void env::Memory::mread(uint8_t* dest, env::guest_t source, uint32_t size, uint32_t usage) const {
	pMapper.mread(dest, source, size, usage);
}
void env::Memory::mwrite(env::guest_t dest, const uint8_t* source, uint32_t size, uint32_t usage) {
	pMapper.mwrite(dest, source, size, usage);
}
