#include "../env-process.h"

uintptr_t env::detail::MemoryAccess::Configure(uint32_t initialPageCount) {
	env::Memory& self = env::Instance()->memory();
	uint32_t caches = env::Instance()->specification().memoryCaches();

	/* allocate the caches for both the guest-application and the internal read/write/code caches and set them up */
	self.pReadCache = caches + 0;
	self.pWriteCache = caches + 1;
	self.pCodeCache = caches + 2;

	/* setup the physical mapping */
	self.pPhysical.push_back(detail::MemoryPhysical{ 0, uint32_t(detail::PhysPageSize * initialPageCount), false });

	/* setup the caches */
	self.pCaches.resize(caches + detail::InternalCaches);

	/* return the highest accessed address */
	return uintptr_t(self.pCaches.data() + self.pCaches.size());
}
uintptr_t env::detail::MemoryAccess::CacheAddress() {
	return uintptr_t(env::Instance()->memory().pCaches.data());
}
size_t env::detail::MemoryAccess::CacheCount() {
	return env::Instance()->memory().pCaches.size();
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
