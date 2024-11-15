#pragma once

#include "../env-common.h"

namespace env::detail {
	struct ProcessAccess {
		static uint32_t PhysicalPages();
		static uint32_t ManagementPages();
		static void AddCoreBinding(const std::u8string& mod, const std::u8string& name);
		static size_t BindingCount();
	};
}
