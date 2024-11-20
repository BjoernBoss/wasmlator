#pragma once

#include "../env-common.h"

namespace env::detail {
	struct MappingAccess {
		static bool CheckLoadable(const std::vector<env::BlockExport>& exports);
		static void BlockLoaded(const std::vector<env::BlockExport>& exports);
		static uintptr_t Configure();
		static uintptr_t CacheAddress();
	};
}
