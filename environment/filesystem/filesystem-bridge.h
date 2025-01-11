#pragma once

#include "../env-common.h"

namespace env::detail {
	struct FileSystemBridge {
		/* exports */
		static void Completed(uint64_t id);

		/* imports */
		static uint64_t QueueTask(const char8_t* task, size_t size);
		static uint64_t ReadResult();
		static bool ReadSuccess();
		static size_t ReadStats();
		static void SelectStat(size_t index);
		static uint64_t ReadStatKey(const char8_t* name, size_t size);

		/* helper */
		static void FreeStatKey(char8_t* data);
	};
}
