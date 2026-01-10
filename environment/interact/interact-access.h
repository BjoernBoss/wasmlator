/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025-2026 Bjoern Boss Henrichsen */
#pragma once

#include "../env-common.h"

namespace env::detail {
	struct InteractAccess {
		static bool CheckVoidIndex(uint32_t index);
		static bool CheckParamIndex(uint32_t index);
		static void DefineExported(const std::u8string& name, uint32_t index, bool param);
		static void CoreLoaded();
	};
}
