#pragma once

#include "../env-common.h"

namespace env::detail {
	struct ProcessAccess {
		static uint32_t PhysicalPages();
		static uint32_t ManagementPages();
	};
}
