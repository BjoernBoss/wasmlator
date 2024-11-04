#pragma once

#include "../env-common.h"

namespace env::detail {
	struct ProcessBridge {
		/* exports */
		static void CoreLoaded(bool succeeded);
		static void BlockLoaded(bool succeeded);

		/* imports */
		static bool Create();
		static void Destroy();
		static bool LoadCore(const uint8_t* data, size_t size, size_t exports);
		static bool LoadBlock(const uint8_t* data, size_t size, size_t exports);
		static void AddCoreExport(const std::u8string_view name, env::guest_t address);
		static void AddBlockExport(const std::u8string_view name, env::guest_t address);
	};
}
