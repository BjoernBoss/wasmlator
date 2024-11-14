#pragma once

#include "../env-common.h"

namespace env::detail {
	struct MappingAccess {
		static bool LoadBlock(const std::vector<uint8_t>& data, const std::vector<env::BlockExport>& exports, uint32_t process);
		static void BlockLoaded(const std::vector<env::BlockExport>& exports);
		static uint32_t AllocateFromManagement(uint32_t address);
		static uint32_t CacheAddress();
		static void CoreLoaded();
	};
}
