#pragma once

#include "../env-common.h"

namespace env::bridge {
	struct Context {
		/* exports */
		static void CoreLoaded(uint64_t process, bool succeeded);
		static void BlockLoaded(uint64_t process, bool succeeded);
		static void Translate(uint64_t process, uint64_t address);
		static void Terminated(uint64_t process, int32_t address);

		/* imports */
		static env::id_t Create(env::Process* process);
		static bool LoadCore(env::id_t id, const uint8_t* data, size_t size);
		static bool LoadBlock(env::id_t id, const uint8_t* data, size_t size, size_t exports);
		static void Destroy(env::id_t id);
		static void AddExport(env::id_t id, const std::u8string_view name, env::guest_t address);
	};
}
