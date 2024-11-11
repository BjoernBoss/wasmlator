#include "../env-process.h"

bool env::detail::InteractAccess::checkVoidIndex(uint32_t index) const {
	return env::Instance()->interact().fCheck(index, false);
}
bool env::detail::InteractAccess::checkParamIndex(uint32_t index) const {
	return env::Instance()->interact().fCheck(index, true);
}
void env::detail::InteractAccess::defineExported(const std::u8string& name, uint32_t index, bool param) {
	(param ? env::Instance()->interact().pCallParam : env::Instance()->interact().pCallVoid).insert({ name, index });
}
void env::detail::InteractAccess::coreLoaded() {
	env::Instance()->interact().pCoreLoaded = true;
	for (const std::u8string& name : env::Instance()->interact().pCoreBound)
		detail::ProcessBridge::DefineCoreBound(env::Instance()->interact().pBlockImportName, name);
}
