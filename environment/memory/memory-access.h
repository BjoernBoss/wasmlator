#pragma once

#include "../env-common.h"

namespace env::detail {
	struct MemoryCache;

	struct MemoryAccess {
		static uintptr_t Configure(uint32_t initialPageCount);
		static uintptr_t CacheAddress();
		static size_t CacheCount();
		static uint32_t ReadCache();
		static uint32_t WriteCache();
		static uint32_t CodeCache();
	};
}
