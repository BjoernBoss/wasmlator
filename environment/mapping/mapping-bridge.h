#pragma once

#include "../env-common.h"

namespace env::detail {
	struct MappingBridge {
		/* exports */
		static uint32_t Resolve(uint64_t address);

		/* imports */
		static bool Reserve(size_t exports);
		static uint32_t Define(const char8_t* name, size_t size, env::guest_t address);
		static env::guest_t Execute(env::guest_t address);
	};
}
