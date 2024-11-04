#include "../env-process.h"

bool env::Memory::mmap(env::guest_t address, uint32_t size, uint32_t usage) {
	return pMapper.mmap(address, size, usage);
}
void env::Memory::munmap(env::guest_t address, uint32_t size) {
	return pMapper.munmap(address, size);
}
void env::Memory::mprotect(env::guest_t address, uint32_t size, uint32_t usage) {
	return pMapper.mprotect(address, size, usage);
}
