/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025 Bjoern Boss Henrichsen */
#pragma once

#include "../env-common.h"

namespace env::detail {
	struct ProcessAccess {
		static bool Setup(env::Process& process, std::unique_ptr<env::System>&& system, uint32_t pageSize, uint32_t memoryCaches, uint32_t contextSize, bool detectWriteExecute, bool logBlocks);
		static uint64_t PhysicalPages();
		static uint64_t MemoryPages();
		static void AddCoreBinding(const std::u8string& mod, const std::u8string& name);
		static void LockBindings();
		static size_t BindingCount();
		static bool HandleTask(const std::u8string& task, std::function<void(std::u8string_view, bool)> callback);
	};
}
