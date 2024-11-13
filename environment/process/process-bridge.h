#pragma once

#include "../env-common.h"

namespace env::detail {
	struct ProcessBridge {
		/* exports */
		static bool CoreLoaded(uint32_t process, bool succeeded);
		static bool BlockLoaded(uint32_t process, bool succeeded);

		/* imports */
		static bool LoadCore(const uint8_t* data, size_t size, uint32_t process);
		static bool SetupCoreFunctions();
		static void DefineCoreBound(const std::u8string& mod, const std::u8string& name);
	};
}
