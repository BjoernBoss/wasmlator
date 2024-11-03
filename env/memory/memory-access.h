#pragma once

#include "../env-common.h"

namespace env::detail {
	class MemoryAccess {
	private:
		env::Process* pProcess = 0;

	public:
		MemoryAccess(env::Process* process);

	public:
		uint32_t configureAndAllocate(uint32_t address, uint32_t caches, uint32_t initialPageCount);
		uint32_t caches() const;
		uint32_t cacheAddress() const;
		uint32_t readCache() const;
		uint32_t writeCache() const;
		uint32_t codeCache() const;
	};
}
