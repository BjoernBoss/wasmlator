#include "../env-process.h"
#include "../interface/interface.h"

void env::detail::ProcessBridge::CoreLoaded(bool succeeded) {
	env::Instance()->fCoreLoaded(succeeded);
}

bool env::detail::ProcessBridge::LoadCore(const uint8_t* data, size_t size) {
	return (host_load_core(data, uint32_t(size)) > 0);
}
bool env::detail::ProcessBridge::SetupCoreFunctions() {
	return (proc_setup_core_functions() > 0);
}
void env::detail::ProcessBridge::DefineCoreBound(const std::u8string& mod, const std::u8string& name) {
	host_define_block_binding(mod.data(), uint32_t(mod.size()), name.data(), uint32_t(name.size()));
}
