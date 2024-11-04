#pragma once

#include "../env-common.h"

namespace env::detail {
	struct ProcessBridge {
		/* exports */
		static void CoreLoaded(bool succeeded);
		static void BlockLoaded(bool succeeded);

		/* imports */
		static bool Create();
		static bool LoadCore(const uint8_t* data, size_t size);
		static bool LoadBlock(const uint8_t* data, size_t size, size_t exports);
		static void Destroy();
		static void AddExport(const std::u8string_view name, env::guest_t address);
	};
}
