/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025 Bjoern Boss Henrichsen */
#pragma once

#include "../env-common.h"

namespace env::detail {
	struct MappingAccess {
		static void CheckLoadable(const std::vector<env::BlockExport>& exports);
		static void BlockLoaded(const std::vector<env::BlockExport>& exports);
	};
}
