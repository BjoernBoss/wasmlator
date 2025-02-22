#pragma once

#include "../env-common.h"

namespace env::detail {
	struct MappingAccess {
		static void CheckLoadable(const std::vector<env::BlockExport>& exports);
		static void BlockLoaded(const std::vector<env::BlockExport>& exports);
	};
}
