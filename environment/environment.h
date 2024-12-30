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
	*	Note: must only be cleared from within coreLoaded/blockLoaded or fully external calls */
	env::Process* Instance();
	bool SetInstance(std::unique_ptr<env::System>&& system, uint32_t pageSize, uint32_t memoryCaches, uint32_t contextSize, bool logBlocks);
	void ClearInstance();
}
