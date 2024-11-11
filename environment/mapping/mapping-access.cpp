#include "../env-process.h"

uint32_t env::detail::MappingAccess::allocateFromManagement(uint32_t address) {
	/* allocate the cache-entries from the management memory */
	env::Instance()->mapping().pCacheAddress = address;
	uint32_t count = (1 << detail::BlockLookupCacheBits) * uint32_t(sizeof(detail::MappingCache));

	/* allocate the 'loading'-state */
	env::Instance()->mapping().pLoadingAddress = address + count;
	return count + sizeof(detail::LoadingState);
}
uint32_t env::detail::MappingAccess::cacheAddress() const {
	return env::Instance()->mapping().pCacheAddress;
}
uint32_t env::detail::MappingAccess::loadingAddress() const {
	return env::Instance()->mapping().pLoadingAddress;
}
void env::detail::MappingAccess::coreLoaded() const {
	detail::ProcessBridge::DefineCoreBound(u8"core", u8"map_functions");
	detail::ProcessBridge::DefineCoreBound(u8"core", u8"map_lookup");
}
