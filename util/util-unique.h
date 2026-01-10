/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025-2026 Bjoern Boss Henrichsen */
#pragma once

#include <cinttypes>

namespace util {
	/* allocate system-wide unique ids (never zero) */
	uint64_t UniqueId();
}
