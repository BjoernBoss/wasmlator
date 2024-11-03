#pragma once

#include "../env-common.h"

namespace env::detail {
	class ProcessAccess {
	private:
		env::Process* pProcess = 0;

	public:
		ProcessAccess(env::Process* process);

	public:
		uint32_t physicalPages() const;
		uint32_t managementPages() const;
	};
}
