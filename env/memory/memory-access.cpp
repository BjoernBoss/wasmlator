#include "../env-process.h"

uint32_t env::detail::MemoryAccess::configureAndAllocate(uint32_t address, uint32_t caches, uint32_t initialPageCount) {
	env::Instance()->memory().pMapper.configure(initialPageCount);
	return env::Instance()->memory().pInteraction.configureAndAllocate(address, caches);
}
uint32_t env::detail::MemoryAccess::caches() const {
	return env::Instance()->memory().pInteraction.caches();
}
uint32_t env::detail::MemoryAccess::cacheAddress() const {
	return env::Instance()->memory().pInteraction.cacheAddress();
}
uint32_t env::detail::MemoryAccess::readCache() const {
	return env::Instance()->memory().pInteraction.readCache();
}
uint32_t env::detail::MemoryAccess::writeCache() const {
	return env::Instance()->memory().pInteraction.writeCache();
}
uint32_t env::detail::MemoryAccess::codeCache() const {
	return env::Instance()->memory().pInteraction.codeCache();
}
