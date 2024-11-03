#pragma once

#include "../env-common.h"

namespace env::bridge {
	struct Context {
		/* exports */
		static void SetExitCode(uint64_t process, int32_t code);

		/* imports */
		static void Read(env::id_t id, uint32_t offset, uint8_t* data, uint32_t size);
		static void Write(env::id_t id, uint32_t offset, const uint8_t* data, uint32_t size);
	};
}
