#pragma once

#include "../env-common.h"

namespace env::detail {
	struct ContextAccess {
		uint32_t configureAndAllocate(uint32_t address);
		uint32_t contextAddress() const;
		uint32_t contextSize() const;
	};
}
