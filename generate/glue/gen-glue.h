/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025-2026 Bjoern Boss Henrichsen */
#pragma once

#include "../gen-common.h"

namespace gen {
	/* No requirements regarding construction of env::Process */
	void SetupGlue(wasm::Module& mod);
}
