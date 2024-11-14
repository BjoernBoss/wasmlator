#pragma once

#include "../env-common.h"

namespace env::detail {
	struct MemoryAccess {
		static uint32_t ConfigureAndAllocate(uint32_t address, uint32_t initialPageCount);
		static uint32_t Caches();
		static uint32_t CacheAddress();
		static uint32_t ReadCache();
		static uint32_t WriteCache();
		static uint32_t CodeCache();
		static void CoreLoaded();
	};
}
