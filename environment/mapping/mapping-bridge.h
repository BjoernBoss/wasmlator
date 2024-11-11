#pragma once

#include "../env-common.h"

namespace env::detail {
	struct MappingBridge {
		/* exports */
		static uint32_t Resolve(uint64_t address);
		static void Flushed();
		static void BlockLoaded(bool succeeded);

		/* imports */
		static bool LoadBlock(const uint8_t* data, size_t size, size_t exports);
		static uint32_t Define(const char8_t* name, size_t size, env::guest_t address);
		static void Flush();
		static uint32_t Execute(env::guest_t address);
	};
}
