#pragma once

#include "../env-common.h"

namespace env::detail {
	struct ProcessBridge {
		/* exports */
		static void TaskCompleted(uint32_t process, std::u8string_view response);

		/* imports */
		static void HandleTask(const std::u8string& task, uint32_t process);
		static bool SetExport(const std::u8string& name, uint32_t index);
		static bool SetupCoreMap();
		static void ResetCoreMap();
		static void BlockImportsPrepare();
		static void BlockImportsNextMember(const std::u8string& name);
		static void BlockImportsSetValue(const std::u8string& name, uint32_t index);
		static void BlockImportsCommit(bool null);
	};
}
