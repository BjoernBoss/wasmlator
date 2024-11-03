#include "../env-process.h"

env::detail::MemoryAccess::MemoryAccess(env::Process* process) : pProcess{ process } {}
uint32_t env::detail::MemoryAccess::configureAndAllocate(uint32_t address, uint32_t caches, uint32_t initialPageCount) {
	pProcess->memory().pMapper.configure(initialPageCount);
	return pProcess->memory().pInteraction.configureAndAllocate(address, caches);
}
uint32_t env::detail::MemoryAccess::caches() const {
	return pProcess->memory().pInteraction.caches();
}
uint32_t env::detail::MemoryAccess::cacheAddress() const {
	return pProcess->memory().pInteraction.cacheAddress();
}
uint32_t env::detail::MemoryAccess::readCache() const {
	return pProcess->memory().pInteraction.readCache();
}
uint32_t env::detail::MemoryAccess::writeCache() const {
	return pProcess->memory().pInteraction.writeCache();
}
uint32_t env::detail::MemoryAccess::codeCache() const {
	return pProcess->memory().pInteraction.codeCache();
}
