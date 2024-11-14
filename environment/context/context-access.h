#pragma once

#include "../env-common.h"

namespace env::detail {
	struct ContextAccess {
		static uint32_t ConfigureAndAllocate(uint32_t address);
		static uint32_t ContextAddress();
		static uint32_t ContextSize();
	};
}
