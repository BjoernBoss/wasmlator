#pragma once

#include "../env-common.h"

namespace env::detail {
	struct ProcessAccess {
		static uint32_t PhysicalPages();
		static uint32_t MemoryPages();
		static void AddCoreBinding(const std::u8string& mod, const std::u8string& name);
		static void LockBindings();
		static size_t BindingCount();
	};
}
