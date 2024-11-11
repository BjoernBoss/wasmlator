#pragma once

#include "../env-common.h"

namespace env::detail {
	struct InteractAccess {
		bool checkVoidIndex(uint32_t index) const;
		bool checkParamIndex(uint32_t index) const;
		void defineExported(const std::u8string& name, uint32_t index, bool param);
		void coreLoaded();
	};
}
