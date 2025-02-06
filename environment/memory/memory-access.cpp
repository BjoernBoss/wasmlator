#include "../environment.h"

static util::Logger logger{ u8"env::memory" };

std::optional<uintptr_t> env::detail::MemoryAccess::Configure(uint64_t& initialPageCount) {
	env::Memory& self = env::Instance()->memory();
	self.pPageSize = env::Instance()->pageSize();
	self.pDetectExecuteWrite = env::Instance()->detectWriteExecute();

	/* compute the number of bits for the offset within each page */
	self.pPageBitShift = 1;
	while ((self.pPageSize >> self.pPageBitShift) > 1)
		++self.pPageBitShift;

	/* ensure the page-size is valid */
	if (detail::PhysPageSize < self.pPageSize || (detail::PhysPageSize % self.pPageSize) != 0) {
		logger.error(u8"The physical page size [", detail::PhysPageSize, u8"] must a multiple of virtual page size [", self.pPageSize, u8']');
		return std::nullopt;
	}

	/* allocate the caches for both the guest-application and the internal read/write/code caches and set them up (times
	*	two as the lower 'cache-count' number are the read-caches, and the upper 'cache-count' number are the write-caches) */
	self.pCacheCount = env::Instance()->memoryCaches();
	uint32_t totalCaches = self.pCacheCount * 2;
	self.pCaches.resize(size_t(totalCaches) + detail::InternalCaches);
	self.pReadCache = totalCaches + 0;
	self.pWriteCache = totalCaches + 1;
	self.pCodeCache = totalCaches + 2;

	/* setup the initial physical page-count and physical mapping */
	initialPageCount = detail::PhysPageCount(detail::InitAllocPages * self.pPageSize);
	self.pPhysical.insert({ 0, detail::MemoryPhysical{ detail::PhysPageSize * initialPageCount, false } });

	/* return the highest accessed address */
	return uintptr_t(self.pCaches.data() + self.pCaches.size());
}
uintptr_t env::detail::MemoryAccess::CacheAddress() {
	return uintptr_t(env::Instance()->memory().pCaches.data());
}
uint32_t env::detail::MemoryAccess::StartOfReadCaches() {
	return 0;
}
uint32_t env::detail::MemoryAccess::StartOfWriteCaches() {
	return env::Instance()->memory().pCacheCount;
}
uint32_t env::detail::MemoryAccess::CacheCount() {
	return env::Instance()->memory().pCacheCount;
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
