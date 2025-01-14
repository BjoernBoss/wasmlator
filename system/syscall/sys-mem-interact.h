#pragma once

#include "../sys-common.h"

#include "sys-file-io.h"

namespace sys::detail {
	class MemoryInteract {
	private:
		struct {
			env::guest_t init = 0;
			env::guest_t current = 0;
			env::guest_t aligned = 0;
		} pBrk;
		env::guest_t pPageSize = 0;

	public:
		MemoryInteract() = default;

	public:
		bool setup(env::guest_t endOfData);
		int64_t brk(env::guest_t address);
		int64_t mmap(env::guest_t address, uint64_t length, uint32_t protect, uint32_t flags, int64_t fd, uint64_t offset);
	};
}
