/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025-2026 Bjoern Boss Henrichsen */
#include "../environment.h"
#include "../host/interface.h"

static util::Logger logger{ u8"env::process" };

void env::detail::ProcessBridge::TaskCompleted(uint32_t process, std::u8string_view response) {
	if (env::Instance() == 0 || !env::Instance()->fTaskCompleted(process, response))
		logger.warn(u8"Old task for [", process, u8"] silently discarded");
}

void env::detail::ProcessBridge::HandleTask(const std::u8string& task, uint32_t process) {
	host_task(task.data(), uint32_t(task.size()), process);
}
bool env::detail::ProcessBridge::SetExport(const std::u8string& name, uint32_t index) {
	return (proc_export(name.data(), uint32_t(name.size()), index) > 0);
}
bool env::detail::ProcessBridge::SetupCoreMap() {
	return (glue_setup_core_map() > 0);
}
void env::detail::ProcessBridge::ResetCoreMap() {
	glue_reset_core_map();
}
void env::detail::ProcessBridge::BlockImportsPrepare() {
	proc_block_imports_prepare();
}
void env::detail::ProcessBridge::BlockImportsNextMember(const std::u8string& name) {
	proc_block_imports_next_member(name.data(), uint32_t(name.size()));
}
void env::detail::ProcessBridge::BlockImportsSetValue(const std::u8string& name, uint32_t index) {
	proc_block_imports_set_value(name.data(), uint32_t(name.size()), index);
}
void env::detail::ProcessBridge::BlockImportsCommit(bool null) {
	proc_block_imports_commit(null ? 1 : 0);
}
