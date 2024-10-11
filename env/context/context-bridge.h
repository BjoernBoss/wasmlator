#pragma once

#include "../env-common.h"

namespace env::bridge {
	struct Context {
		/* exports */
		static void CoreLoaded(uint64_t process, bool succeeded);

		/* imports */
		static env::id_t Create(env::Process* process);
		static bool SetCore(env::id_t id, const uint8_t* data, size_t size);
		static void Destroy(env::id_t id);
	};
}
