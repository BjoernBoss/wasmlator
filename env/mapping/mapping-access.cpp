#include "../env-process.h"

uint32_t env::detail::MappingAccess::allocateFromManagement(uint32_t address) {
	/* allocate the cache-entries from the management memory */
	env::Instance()->mapping().pCacheAddress = address;
	return (1 << detail::BlockLookupCacheBits) * uint32_t(sizeof(detail::MappingCache));
}
uint32_t env::detail::MappingAccess::cacheAddress() const {
	return env::Instance()->mapping().pCacheAddress;
}
