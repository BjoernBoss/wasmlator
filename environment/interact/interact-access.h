#pragma once

#include "../env-common.h"

namespace env::detail {
	struct InteractAccess {
		static bool CheckVoidIndex(uint32_t index);
		static bool CheckParamIndex(uint32_t index);
		static void DefineExported(const std::u8string& name, uint32_t index, bool param);
		static void FinalizingCore();
		static void CoreLoaded();
	};
}
