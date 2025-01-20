#pragma once

#include "../env-common.h"

namespace env::detail {
	struct MemoryAccess {
		static std::optional<uintptr_t> Configure(uint64_t& initialPageCount);
		static uintptr_t CacheAddress();
		static size_t CacheCount();
		static uint32_t ReadCache();
		static uint32_t WriteCache();
		static uint32_t CodeCache();
	};
}
