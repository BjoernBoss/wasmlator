#pragma once

#include "../env-common.h"

namespace env::detail {
	struct MappingAccess {
		bool loadBlock(const std::vector<uint8_t>& data, const std::vector<env::BlockExport>& exports, uint32_t process);
		void blockLoaded(const std::vector<env::BlockExport>& exports);
		uint32_t allocateFromManagement(uint32_t address);
		uint32_t cacheAddress() const;
		void coreLoaded() const;
	};
}
