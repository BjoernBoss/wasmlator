#pragma once

#include <ustring/ustring.h>
#include <cinttypes>
#include <string>
#include <type_traits>

#include "../util/logging.h"
#include "../wasm/wasm.h"

namespace env {
	static constexpr uint64_t PageSize = 0x1000;
	static constexpr uint32_t MinPages = 5'000;
	static constexpr uint32_t PageCount(uint64_t bytes) {
		return uint32_t((bytes + env::PageSize - 1) / env::PageSize);
	}
}
