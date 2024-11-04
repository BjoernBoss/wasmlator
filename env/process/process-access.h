#pragma once

#include "../env-common.h"

namespace env::detail {
	struct ProcessAccess {
		uint32_t physicalPages() const;
		uint32_t managementPages() const;
	};
}
