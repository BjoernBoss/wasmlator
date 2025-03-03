/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025 Bjoern Boss Henrichsen */
#pragma once

#include "../env-common.h"

namespace env::detail {
	struct ContextAccess {
		static uintptr_t Configure();
		static uintptr_t ContextAddress();
		static size_t ContextSize();
	};
}
