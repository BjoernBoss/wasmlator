#include "../env-process.h"
#include "../interface/interface.h"

void env::detail::ProcessBridge::CoreLoaded(bool succeeded) {
	env::Instance()->fCoreLoaded(succeeded);
}
void env::detail::ProcessBridge::BlockLoaded(bool succeeded) {
	env::Instance()->fBlockLoaded(succeeded);
}

bool env::detail::ProcessBridge::Create() {
	return (proc_create() > 0);
}
bool env::detail::ProcessBridge::LoadCore(const uint8_t* data, size_t size) {
	return (proc_load_core(data, uint32_t(size)) > 0);
}
bool env::detail::ProcessBridge::LoadBlock(const uint8_t* data, size_t size, size_t exports) {
	return (proc_load_block(data, uint32_t(size), uint32_t(exports)) > 0);
}
void env::detail::ProcessBridge::Destroy() {
	proc_destroy();
}
void env::detail::ProcessBridge::AddExport(const std::u8string_view name, env::guest_t address) {
	proc_add_export(name.data(), uint32_t(name.size()), address);
}
