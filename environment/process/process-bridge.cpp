#include "../environment.h"
#include "../host/interface.h"

static host::Logger logger{ u8"env::process" };

void env::detail::ProcessBridge::CoreLoaded(uint32_t process) {
	if (env::Instance() == 0 || !env::Instance()->fCoreLoaded(process))
		logger.warn(u8"Old core-load for [", process, u8"] silently discarded");
}
void env::detail::ProcessBridge::BlockLoaded(uint32_t process) {
	if (env::Instance() == 0 || !env::Instance()->fBlockLoaded(process))
		logger.warn(u8"Old block-load for [", process, u8"] silently discarded");
}

bool env::detail::ProcessBridge::LoadCore(const std::vector<uint8_t>& data, uint32_t process) {
	return (host_load_core(data.data(), uint32_t(data.size()), process) > 0);
}
bool env::detail::ProcessBridge::LoadBlock(const std::vector<uint8_t>& data, uint32_t process) {
	return (host_load_block(data.data(), uint32_t(data.size()), process) > 0);
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
