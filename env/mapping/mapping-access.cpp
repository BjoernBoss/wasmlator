#include "../env-process.h"

env::detail::MappingAccess::MappingAccess(env::Process* process) : pProcess{ process } {}
uint32_t env::detail::MappingAccess::allocateFromManagement(uint32_t address) {
	/* allocate the cache-entries from the management memory */
	pProcess->mapping().pCacheAddress = address;
	return (1 << detail::BlockLookupCacheBits) * uint32_t(sizeof(detail::MappingCache));
}
uint32_t env::detail::MappingAccess::cacheAddress() const {
	return pProcess->mapping().pCacheAddress;
}
