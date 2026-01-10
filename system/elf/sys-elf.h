/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025-2026 Bjoern Boss Henrichsen */
#pragma once

#include "sys-elf-types.h"
#include "sys-load-elf.h"

namespace sys {
	/* load the elf-file into the current environment (requires env::Process to be set-up) */
	elf::LoadState LoadElf(const uint8_t* data, size_t size);

	/* continue loading of elf after the requested interpreter has been fetched
	*	(requires initial sys::LoadElf call with a non-empty interpreter path) */
	void LoadElfInterpreter(elf::LoadState& state, const uint8_t* data, size_t size);
}
