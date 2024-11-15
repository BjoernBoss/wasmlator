#include "../env-process.h"

bool env::detail::MappingAccess::CheckLoadable(const std::vector<env::BlockExport>& exports) {
	return env::Instance()->mapping().fCheckLoadable(exports);
}
void env::detail::MappingAccess::BlockLoaded(const std::vector<env::BlockExport>& exports) {
	env::Instance()->mapping().fBlockExports(exports);
}
uint32_t env::detail::MappingAccess::AllocateFromManagement(uint32_t address) {
	/* allocate the cache-entries from the management memory */
	env::Instance()->mapping().pCacheAddress = address;
	return (1 << detail::BlockLookupCacheBits) * uint32_t(sizeof(detail::MappingCache));
}
uint32_t env::detail::MappingAccess::CacheAddress() {
	return env::Instance()->mapping().pCacheAddress;
}
