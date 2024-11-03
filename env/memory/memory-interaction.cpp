#include "../env-process.h"

env::detail::MemoryInteraction::MemoryInteraction(env::Process* process) : pProcess{ process } {}

uint32_t env::detail::MemoryInteraction::fReadi32Fromi8(env::guest_t address) const {
	return bridge::Memory::Readi32Fromi8(pProcess->id(), address);
}
uint32_t env::detail::MemoryInteraction::fReadi32Fromu8(env::guest_t address) const {
	return bridge::Memory::Readi32Fromu8(pProcess->id(), address);
}
uint32_t env::detail::MemoryInteraction::fReadi32Fromi16(env::guest_t address) const {
	return bridge::Memory::Readi32Fromi16(pProcess->id(), address);
}
uint32_t env::detail::MemoryInteraction::fReadi32Fromu16(env::guest_t address) const {
	return bridge::Memory::Readi32Fromu16(pProcess->id(), address);
}
uint32_t env::detail::MemoryInteraction::fReadi32(env::guest_t address) const {
	return bridge::Memory::Readi32(pProcess->id(), address);
}
uint64_t env::detail::MemoryInteraction::fReadi64(env::guest_t address) const {
	return bridge::Memory::Readi64(pProcess->id(), address);
}
float env::detail::MemoryInteraction::fReadf32(env::guest_t address) const {
	return bridge::Memory::Readf32(pProcess->id(), address);
}
double env::detail::MemoryInteraction::fReadf64(env::guest_t address) const {
	return bridge::Memory::Readf64(pProcess->id(), address);
}
void env::detail::MemoryInteraction::fWritei32Fromi8(env::guest_t address, uint32_t value) const {
	bridge::Memory::Writei32Fromi8(pProcess->id(), address, value);
}
void env::detail::MemoryInteraction::fWritei32Fromu8(env::guest_t address, uint32_t value) const {
	bridge::Memory::Writei32Fromu8(pProcess->id(), address, value);
}
void env::detail::MemoryInteraction::fWritei32Fromi16(env::guest_t address, uint32_t value) const {
	bridge::Memory::Writei32Fromi16(pProcess->id(), address, value);
}
void env::detail::MemoryInteraction::fWritei32Fromu16(env::guest_t address, uint32_t value) const {
	bridge::Memory::Writei32Fromu16(pProcess->id(), address, value);
}
void env::detail::MemoryInteraction::fWritei32(env::guest_t address, uint32_t value) const {
	bridge::Memory::Writei32(pProcess->id(), address, value);
}
void env::detail::MemoryInteraction::fWritei64(env::guest_t address, uint64_t value) const {
	bridge::Memory::Writei64(pProcess->id(), address, value);
}
void env::detail::MemoryInteraction::fWritef32(env::guest_t address, float value) const {
	bridge::Memory::Writef32(pProcess->id(), address, value);
}
void env::detail::MemoryInteraction::fWritef64(env::guest_t address, double value) const {
	bridge::Memory::Writef64(pProcess->id(), address, value);
}
uint32_t env::detail::MemoryInteraction::fCodei32Fromi8(env::guest_t address) const {
	return bridge::Memory::Codei32Fromi8(pProcess->id(), address);
}
uint32_t env::detail::MemoryInteraction::fCodei32Fromu8(env::guest_t address) const {
	return bridge::Memory::Codei32Fromu8(pProcess->id(), address);
}
uint32_t env::detail::MemoryInteraction::fCodei32Fromi16(env::guest_t address) const {
	return bridge::Memory::Codei32Fromi16(pProcess->id(), address);
}
uint32_t env::detail::MemoryInteraction::fCodei32Fromu16(env::guest_t address) const {
	return bridge::Memory::Codei32Fromu16(pProcess->id(), address);
}
uint32_t env::detail::MemoryInteraction::fCodei32(env::guest_t address) const {
	return bridge::Memory::Codei32(pProcess->id(), address);
}
uint64_t env::detail::MemoryInteraction::fCodei64(env::guest_t address) const {
	return bridge::Memory::Codei64(pProcess->id(), address);
}
float env::detail::MemoryInteraction::fCodef32(env::guest_t address) const {
	return bridge::Memory::Codef32(pProcess->id(), address);
}
double env::detail::MemoryInteraction::fCodef64(env::guest_t address) const {
	return bridge::Memory::Codef64(pProcess->id(), address);
}

uint32_t env::detail::MemoryInteraction::configureAndAllocate(uint32_t address, uint32_t caches) {
	/* setup the cache-indices for both the guest-application and the internal read/write/code caches */
	pCacheCount = caches;
	pReadCache = pCacheCount + 0;
	pWriteCache = pCacheCount + 1;
	pCodeCache = pCacheCount + 2;

	/* allocate the cache-entries from the management memory */
	pCacheAddress = address;
	return (pCacheCount + detail::InternalCaches) * uint32_t(sizeof(detail::MemoryCache));
}
uint32_t env::detail::MemoryInteraction::caches() const {
	return pCacheCount;
}
uint32_t env::detail::MemoryInteraction::cacheAddress() const {
	return pCacheAddress;
}
uint32_t env::detail::MemoryInteraction::readCache() const {
	return pReadCache;
}
uint32_t env::detail::MemoryInteraction::writeCache() const {
	return pWriteCache;
}
uint32_t env::detail::MemoryInteraction::codeCache() const {
	return pCodeCache;
}
