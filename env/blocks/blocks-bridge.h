#pragma once

#include "../env-common.h"

namespace env::bridge {
	struct Blocks {
		/* exports */
		static uint32_t LookupComplex(uint64_t process, uint64_t address);

		/* imports */
		static void FlushBlocks(env::id_t id);
	};
}
