#pragma once

#include "../env-common.h"

namespace env::detail {
	struct ContextBridge {
		/* exports */
		static void SetExitCode(int32_t code);

		/* imports */
		static void Read(uint32_t offset, uint8_t* data, uint32_t size);
		static void Write(uint32_t offset, const uint8_t* data, uint32_t size);
	};
}
