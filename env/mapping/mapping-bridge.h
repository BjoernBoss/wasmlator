#pragma once

#include "../env-common.h"

namespace env::detail {
	struct MappingBridge {
		/* exports */
		static uint32_t Resolve(uint64_t address);
		static void Flushed();
		static void Associate(uint64_t address, uint32_t index);

		/* imports */
		static uint32_t Execute(env::guest_t address);
		static void Flush();
	};
}
