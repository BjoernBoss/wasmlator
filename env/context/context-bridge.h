#pragma once

#include "env-context.h"

namespace env::bridge {
	struct Context {
		/* exports */
		static void CoreLoaded(uint64_t self, bool succeeded);

		/* imports */
		static env::id_t Create(env::Context* self);
		static bool SetCore(env::id_t id, const uint8_t* data, size_t size);
		static void Destroy(env::id_t id);
	};
}
