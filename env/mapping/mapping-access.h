#pragma once

#include "../env-common.h"

namespace env::detail {
	class MappingAccess {
	private:
		env::Process* pProcess = 0;

	public:
		MappingAccess(env::Process* process);

	public:
		uint32_t allocateFromManagement(uint32_t address);
		uint32_t cacheAddress() const;
	};
}
