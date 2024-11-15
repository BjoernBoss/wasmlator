#include "../env-process.h"
#include "../interface/interface.h"

bool env::detail::ProcessBridge::CoreLoaded(uint32_t process, bool succeeded) {
	if (env::Instance() != 0 && env::Instance()->fCoreLoaded(process, succeeded))
		return true;
	host::Debug(u8"Old core-load for [", process, u8"] silently discarded");
	return false;
}
bool env::detail::ProcessBridge::BlockLoaded(uint32_t process, bool succeeded) {
	if (env::Instance() != 0 && env::Instance()->fBlockLoaded(process, succeeded))
		return true;
	host::Debug(u8"Old block-load for [", process, u8"] silently discarded");
	return false;
}

bool env::detail::ProcessBridge::LoadCore(const uint8_t* data, size_t size, uint32_t process) {
	return (host_load_core(data, uint32_t(size), process) > 0);
}
bool env::detail::ProcessBridge::SetupCoreFunctions() {
	return (proc_setup_core_functions() > 0);
}
void env::detail::ProcessBridge::DefineCoreBound(const std::u8string& mod, const std::u8string& name) {
	host_define_block_binding(mod.data(), uint32_t(mod.size()), name.data(), uint32_t(name.size()));
}
