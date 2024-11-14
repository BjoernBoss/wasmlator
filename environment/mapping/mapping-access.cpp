#include "../env-process.h"

bool env::detail::MappingAccess::LoadBlock(const std::vector<uint8_t>& data, const std::vector<env::BlockExport>& exports, uint32_t process) {
	return env::Instance()->mapping().fLoadBlock(data, exports, process);
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
void env::detail::MappingAccess::CoreLoaded() {
	detail::ProcessBridge::DefineCoreBound(u8"core", u8"map_functions");
	detail::ProcessBridge::DefineCoreBound(u8"core", u8"map_lookup");
}
