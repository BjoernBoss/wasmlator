#pragma once

#include "../env-common.h"

namespace env::detail {
	struct MappingAccess {
		static bool CheckLoadable(const std::vector<env::BlockExport>& exports);
		static void BlockLoaded(const std::vector<env::BlockExport>& exports);
		static uint32_t AllocateFromManagement(uint32_t address);
		static uint32_t CacheAddress();
	};
}
