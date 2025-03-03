/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025 Bjoern Boss Henrichsen */
#pragma once

#include "../env-common.h"

namespace env::detail {
	struct MemoryAccess {
		static std::optional<uintptr_t> Configure(uint64_t& initialPageCount);
		static uintptr_t CacheAddress();
		static uint32_t StartOfReadCaches();
		static uint32_t StartOfWriteCaches();
		static uint32_t CacheCount();
		static uint32_t ReadCache();
		static uint32_t WriteCache();
		static uint32_t CodeCache();
	};
}
