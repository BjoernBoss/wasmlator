#pragma once

#include "../env-common.h"

namespace env::detail {
	struct MappingAccess {
		uint32_t allocateFromManagement(uint32_t address);
		uint32_t cacheAddress() const;
	};
}
