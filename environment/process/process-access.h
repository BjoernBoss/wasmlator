#pragma once

#include "../env-common.h"

namespace env::detail {
	struct ProcessAccess {
		static bool Setup(env::Process& process, std::unique_ptr<env::System>&& system, uint32_t pageSize, uint32_t memoryCaches, uint32_t contextSize, bool logBlocks);
		static uint32_t PhysicalPages();
		static uint32_t MemoryPages();
		static void AddCoreBinding(const std::u8string& mod, const std::u8string& name);
		static void LockBindings();
		static size_t BindingCount();
		static void HandleTask(const std::u8string& task, std::function<void(std::u8string_view)> callback);
	};
}
