#pragma once

#include "env-common.h"
#include "process/env-process.h"
#include "context/env-context.h"
#include "mapping/env-mapping.h"
#include "memory/env-memory.h"
#include "interact/env-interact.h"

namespace env {
	/* Many operations on a process must first be called after the core has been loaded, as they might internally
	*		interact with the core, which will otherwise result in a null-function execution within the glue-module
	*	Note: must only be destroyed form within coreLoaded/blockLoaded or the main startup */
	env::Process* Instance();
}
