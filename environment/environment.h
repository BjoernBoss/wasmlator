/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025-2026 Bjoern Boss Henrichsen */
#pragma once

#include "env-common.h"
#include "process/env-process.h"
#include "context/env-context.h"
#include "mapping/env-mapping.h"
#include "memory/env-memory.h"
#include "interact/env-interact.h"

namespace env {
	/* Many operations on a process must first be called after the core has been loaded, as they might internally
	*		interact with the core, which will otherwise result in a null-function execution within the glue-module.
	*	Note: shutdown must be performed through env::System::shutdown, and must only be exeucted from within external calls */
	env::Process* Instance();
	bool SetInstance(std::unique_ptr<env::System>&& system, uint32_t pageSize, uint32_t memoryCaches, uint32_t contextSize, bool detectWriteExecute, bool logBlocks);
	void ClearInstance();
}
