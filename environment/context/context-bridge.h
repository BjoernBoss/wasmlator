#pragma once

#include "../env-common.h"

namespace env::detail {
	struct ContextBridge {
		/* exports */
		static void Terminate(int32_t code);
		static void NotDecodable(uint64_t address);
		static void NotReachable(uint64_t address);

		/* imports */
		static uint64_t Read(uint32_t offset, uint32_t size);
		static void Write(uint32_t offset, uint32_t size, uint64_t value);
	};
}
