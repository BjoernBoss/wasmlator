#pragma once

#include "../env-common.h"

namespace env::detail {
	struct ContextBridge {
		/* exports */
		static void Terminate(int32_t code);

		/* imports */
		static uint64_t Read(uint32_t offset, uint32_t size);
		static void Write(uint32_t offset, uint32_t size, uint64_t value);
	};
}
