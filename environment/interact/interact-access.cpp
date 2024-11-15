#include "../env-process.h"

bool env::detail::InteractAccess::CheckVoidIndex(uint32_t index) {
	return env::Instance()->interact().fCheck(index, false);
}
bool env::detail::InteractAccess::CheckParamIndex(uint32_t index) {
	return env::Instance()->interact().fCheck(index, true);
}
void env::detail::InteractAccess::DefineExported(const std::u8string& name, uint32_t index, bool param) {
	(param ? env::Instance()->interact().pCallParam : env::Instance()->interact().pCallVoid).insert({ name, index });
}
void env::detail::InteractAccess::FinalizingCore() {
	env::Instance()->interact().pCoreState = env::Interact::LoadingState::loading;
}

void env::detail::InteractAccess::CoreLoaded() {
	env::Instance()->interact().pCoreState = env::Interact::LoadingState::loaded;
}
