#pragma once

#include "../env-common.h"

namespace env::detail {
	struct ContextBridge {
		/* exports */
		static void Terminate(int32_t code, uint64_t address);
		static void NotDecodable(uint64_t address);
		static void NotReachable(uint64_t address);
	};
}
