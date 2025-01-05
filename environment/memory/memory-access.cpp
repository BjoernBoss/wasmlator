#include "../environment.h"

static host::Logger logger{ u8"env::memory" };

std::optional<uintptr_t> env::detail::MemoryAccess::Configure(uint32_t& initialPageCount) {
	env::Memory& self = env::Instance()->memory();
	uint32_t caches = env::Instance()->memoryCaches();
	self.pPageSize = env::Instance()->pageSize();

	/* ensure the page-size is valid */
	if (detail::PhysPageSize < self.pPageSize || (detail::PhysPageSize % self.pPageSize) != 0) {
		logger.error(u8"The physical page size [", detail::PhysPageSize, u8"] must a multiple of virtual page size [", self.pPageSize, u8']');
		return std::nullopt;
	}

	/* allocate the caches for both the guest-application and the internal read/write/code caches and set them up */
	self.pReadCache = caches + 0;
	self.pWriteCache = caches + 1;
	self.pCodeCache = caches + 2;

	/* setup the initial physical page-count and physical mapping */
	initialPageCount = detail::PhysPageCount(uint64_t(detail::InitAllocPages * self.pPageSize));
	self.pPhysical.push_back(detail::MemoryPhysical{ 0, uint32_t(detail::PhysPageSize * initialPageCount), false });

	/* setup the caches */
	self.pCaches.resize(size_t(caches) + detail::InternalCaches);

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
