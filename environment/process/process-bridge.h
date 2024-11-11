#pragma once

#include "../env-common.h"

namespace env::detail {
	struct ProcessBridge {
		/* exports */
		static void CoreLoaded(bool succeeded);

		/* imports */
		static bool LoadCore(const uint8_t* data, size_t size);
		static bool SetupCoreFunctions();
	};
}
