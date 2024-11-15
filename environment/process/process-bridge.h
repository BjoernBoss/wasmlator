#pragma once

#include "../env-common.h"

namespace env::detail {
	struct ProcessBridge {
		/* exports */
		static void CoreLoaded(uint32_t process, bool succeeded);
		static void BlockLoaded(uint32_t process, bool succeeded);

		/* imports */
		static bool LoadCore(const std::vector<uint8_t>& data, uint32_t process);
		static bool LoadBlock(const std::vector<uint8_t>& data, uint32_t process);
		static bool SetExport(const std::u8string& name, uint32_t index);
		static bool SetupCoreMap();
		static void Reset();
	};
}
