#pragma once

#include <cinttypes>
#include <string>

#include "../util/logging.h"
#include "../wasm/wasm.h"

namespace env {
	using addr_t = uint64_t;
	using physical_t = uint32_t;

	using module_t = uint32_t;

	static constexpr uint64_t PageSize = 0x1000;
	static constexpr uint32_t MinPages = 5'000;
	static constexpr uint32_t PageCount(uint64_t bytes) {
		return uint32_t((bytes + env::PageSize - 1) / env::PageSize);
	}


	class Process;
	class Memory;
}
