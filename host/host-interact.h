/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025-2026 Bjoern Boss Henrichsen */
#pragma once

#include <cinttypes>
#include <string>
#include <ustring/ustring.h>

namespace host {
	/* fetch a random 32-bit value */
	uint32_t GetRandom();

	/* fetch the current time in [us] */
	uint64_t GetStampUS();

	/* fetch the current timezone east of utc in [min] */
	int32_t GetTimeZoneMIN();

	/* notify the host to check the performance metrics */
	void HostCheckMetrics();

	/* direct logs to output without logging wrapper */
	void PrintOut(std::u8string_view msg);

	/* direct logs to output without logging wrapper */
	void PrintOutLn(std::u8string_view msg);
}
