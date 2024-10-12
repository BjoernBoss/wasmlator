#pragma once

#include "../env-common.h"

namespace env::bridge {
	struct Blocks {
		/* exports */
		static uint32_t LookupComplex(uint64_t process, uint64_t address);
		static void Flushed(uint64_t process);
		static void Associate(uint64_t process, uint64_t address, uint32_t index);

		/* imports */
		static void Execute(env::id_t id, env::guest_t address);
		static void FlushBlocks(env::id_t id);
	};
}
