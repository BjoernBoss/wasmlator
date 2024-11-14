#include "../env-process.h"

uint32_t env::detail::MemoryAccess::ConfigureAndAllocate(uint32_t address, uint32_t initialPageCount) {
	env::Instance()->memory().pMapper.configure(initialPageCount);

	/* setup the cache-indices for both the guest-application and the internal read/write/code caches */
	env::Instance()->memory().pCacheCount = env::Instance()->specification().memoryCaches();
	env::Instance()->memory().pReadCache = env::Instance()->memory().pCacheCount + 0;
	env::Instance()->memory().pWriteCache = env::Instance()->memory().pCacheCount + 1;
	env::Instance()->memory().pCodeCache = env::Instance()->memory().pCacheCount + 2;

	/* allocate the cache-entries from the management memory */
	env::Instance()->memory().pCacheAddress = address;
	return (env::Instance()->memory().pCacheCount + detail::InternalCaches) * uint32_t(sizeof(detail::MemoryCache));
}
uint32_t env::detail::MemoryAccess::Caches() {
	return env::Instance()->memory().pCacheCount;
}
uint32_t env::detail::MemoryAccess::CacheAddress() {
	return env::Instance()->memory().pCacheAddress;
}
uint32_t env::detail::MemoryAccess::ReadCache() {
	return env::Instance()->memory().pReadCache;
}
uint32_t env::detail::MemoryAccess::WriteCache() {
	return env::Instance()->memory().pWriteCache;
}
uint32_t env::detail::MemoryAccess::CodeCache() {
	return env::Instance()->memory().pCodeCache;
}
void env::detail::MemoryAccess::CoreLoaded() {
	detail::ProcessBridge::DefineCoreBound(u8"core", u8"mem_lookup_read");
	detail::ProcessBridge::DefineCoreBound(u8"core", u8"mem_lookup_write");
	detail::ProcessBridge::DefineCoreBound(u8"core", u8"mem_lookup_code");
}
