#pragma once

#include "../env-common.h"

namespace env::detail {
	struct ContextBridge {
		/* exports */
		static void Terminate(int32_t code, uint64_t address);
		static void CodeException(uint64_t address, uint32_t id);
	};
}
