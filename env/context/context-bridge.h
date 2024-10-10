#pragma once

#include "env-context.h"

namespace env::bridge {
	struct Context {
		/* imports */
		static env::id_t Create();
		static bool SetCore(env::id_t id, const uint8_t* data, size_t size);
		static void Destroy(env::id_t id);
	};
}
