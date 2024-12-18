#pragma once

#include "../env-common.h"

namespace env::detail {
	struct ProcessBridge {
		/* exports */
		static void CoreLoaded(uint32_t process, bool succeeded);
		static void BlockLoaded(uint32_t process, bool succeeded);
		static void Terminate(int32_t code, uint64_t address);
		static void NotDecodable(uint64_t address);
		static void NotReachable(uint64_t address);
		static void SingleStep(uint64_t address);

		/* imports */
		static bool LoadCore(const std::vector<uint8_t>& data, uint32_t process);
		static bool LoadBlock(const std::vector<uint8_t>& data, uint32_t process);
		static bool SetExport(const std::u8string& name, uint32_t index);
		static bool SetupCoreMap();
		static void ResetCoreMap();
		static void BlockImportsPrepare();
		static void BlockImportsNextMember(const std::u8string& name);
		static void BlockImportsSetValue(const std::u8string& name, uint32_t index);
		static void BlockImportsCommit(bool null);
	};
}
