/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025 Bjoern Boss Henrichsen */
#include "util-unique.h"

namespace global {
	static uint64_t NextId = 0;
}

uint64_t util::UniqueId() {
	return ++global::NextId;
}
