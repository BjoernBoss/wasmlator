#pragma once

#include "../env-common.h"

namespace env::detail {
	struct MemoryAccess {
		uint32_t configureAndAllocate(uint32_t address, uint32_t initialPageCount);
		uint32_t caches() const;
		uint32_t cacheAddress() const;
		uint32_t readCache() const;
		uint32_t writeCache() const;
		uint32_t codeCache() const;
		void coreLoaded() const;
	};
}
