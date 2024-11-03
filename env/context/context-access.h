#pragma once

#include "../env-common.h"

namespace env::detail {
	class ContextAccess {
	private:
		env::Process* pProcess = 0;

	public:
		ContextAccess(env::Process* process);

	public:
		uint32_t configureAndAllocate(uint32_t address, uint32_t context);
		uint32_t contextAddress() const;
		uint32_t contextSize() const;
	};
}
