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
