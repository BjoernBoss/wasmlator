#pragma once

#include <cinttypes>

namespace util {
	/* allocate system-wide unique ids (never zero) */
	uint64_t UniqueId();
}
