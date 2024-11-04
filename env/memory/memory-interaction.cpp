#include "../env-process.h"

uint32_t env::detail::MemoryInteraction::fReadi32Fromi8(env::guest_t address) const {
	return detail::MemoryBridge::Readi32Fromi8(address);
}
uint32_t env::detail::MemoryInteraction::fReadi32Fromu8(env::guest_t address) const {
	return detail::MemoryBridge::Readi32Fromu8(address);
}
uint32_t env::detail::MemoryInteraction::fReadi32Fromi16(env::guest_t address) const {
	return detail::MemoryBridge::Readi32Fromi16(address);
}
uint32_t env::detail::MemoryInteraction::fReadi32Fromu16(env::guest_t address) const {
	return detail::MemoryBridge::Readi32Fromu16(address);
}
uint32_t env::detail::MemoryInteraction::fReadi32(env::guest_t address) const {
	return detail::MemoryBridge::Readi32(address);
}
uint64_t env::detail::MemoryInteraction::fReadi64(env::guest_t address) const {
	return detail::MemoryBridge::Readi64(address);
}
float env::detail::MemoryInteraction::fReadf32(env::guest_t address) const {
	return detail::MemoryBridge::Readf32(address);
}
double env::detail::MemoryInteraction::fReadf64(env::guest_t address) const {
	return detail::MemoryBridge::Readf64(address);
}
void env::detail::MemoryInteraction::fWritei32Fromi8(env::guest_t address, uint32_t value) const {
	detail::MemoryBridge::Writei32Fromi8(address, value);
}
void env::detail::MemoryInteraction::fWritei32Fromu8(env::guest_t address, uint32_t value) const {
	detail::MemoryBridge::Writei32Fromu8(address, value);
}
void env::detail::MemoryInteraction::fWritei32Fromi16(env::guest_t address, uint32_t value) const {
	detail::MemoryBridge::Writei32Fromi16(address, value);
}
void env::detail::MemoryInteraction::fWritei32Fromu16(env::guest_t address, uint32_t value) const {
	detail::MemoryBridge::Writei32Fromu16(address, value);
}
void env::detail::MemoryInteraction::fWritei32(env::guest_t address, uint32_t value) const {
	detail::MemoryBridge::Writei32(address, value);
}
void env::detail::MemoryInteraction::fWritei64(env::guest_t address, uint64_t value) const {
	detail::MemoryBridge::Writei64(address, value);
}
void env::detail::MemoryInteraction::fWritef32(env::guest_t address, float value) const {
	detail::MemoryBridge::Writef32(address, value);
}
void env::detail::MemoryInteraction::fWritef64(env::guest_t address, double value) const {
	detail::MemoryBridge::Writef64(address, value);
}
uint32_t env::detail::MemoryInteraction::fCodei32Fromi8(env::guest_t address) const {
	return detail::MemoryBridge::Codei32Fromi8(address);
}
uint32_t env::detail::MemoryInteraction::fCodei32Fromu8(env::guest_t address) const {
	return detail::MemoryBridge::Codei32Fromu8(address);
}
uint32_t env::detail::MemoryInteraction::fCodei32Fromi16(env::guest_t address) const {
	return detail::MemoryBridge::Codei32Fromi16(address);
}
uint32_t env::detail::MemoryInteraction::fCodei32Fromu16(env::guest_t address) const {
	return detail::MemoryBridge::Codei32Fromu16(address);
}
uint32_t env::detail::MemoryInteraction::fCodei32(env::guest_t address) const {
	return detail::MemoryBridge::Codei32(address);
}
uint64_t env::detail::MemoryInteraction::fCodei64(env::guest_t address) const {
	return detail::MemoryBridge::Codei64(address);
}
float env::detail::MemoryInteraction::fCodef32(env::guest_t address) const {
	return detail::MemoryBridge::Codef32(address);
}
double env::detail::MemoryInteraction::fCodef64(env::guest_t address) const {
	return detail::MemoryBridge::Codef64(address);
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
