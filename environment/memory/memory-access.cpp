#include "../env-process.h"

uintptr_t env::detail::MemoryAccess::Configure(uint32_t initialPageCount) {
	/* allocate the caches for both the guest-application and the internal read/write/code caches and set them up */
	env::Instance()->memory().pReadCache = env::Instance()->specification().memoryCaches() + 0;
	env::Instance()->memory().pWriteCache = env::Instance()->specification().memoryCaches() + 1;
	env::Instance()->memory().pCodeCache = env::Instance()->specification().memoryCaches() + 2;

	/* setup the mapper and return the highest accessed address used by the memory */
	return env::Instance()->memory().pMapper.configure(initialPageCount, env::Instance()->specification().memoryCaches() + detail::InternalCaches);
}
uintptr_t env::detail::MemoryAccess::CacheAddress() {
	return uintptr_t(env::Instance()->memory().pMapper.pCaches.data());
}
size_t env::detail::MemoryAccess::CacheCount() {
	return env::Instance()->memory().pMapper.pCaches.size();
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
