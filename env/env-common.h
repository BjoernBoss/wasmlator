#pragma once

#include <ustring/ustring.h>
#include <cinttypes>
#include <string>
#include <type_traits>

#include "../util/logging.h"
#include "../wasm/wasm.h"

namespace env {
	static constexpr uint32_t MinPages = 5'000;
	static constexpr uint32_t MinUpscalePages = 512;

	static constexpr uint64_t PageSize = 0x1000;
	static constexpr uint32_t PageCount(uint64_t bytes) {
		return uint32_t((bytes + env::PageSize - 1) / env::PageSize);
	}
	static constexpr uint32_t PageOffset(uint64_t address) {
		return uint32_t(address & (env::PageSize - 1));
	}
	static constexpr uint64_t PageIndex(uint64_t address) {
		return (address / env::PageSize);
	}
}
