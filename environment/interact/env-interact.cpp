#include "../env-process.h"

bool env::Interact::fCheck(uint32_t index, bool param) const {
	return (index < (param ? pParamCallbacks.size() : pVoidCallbacks.size()));
}
void env::Interact::fInvoke(uint32_t index) const {
	pVoidCallbacks[index]();
}
uint64_t env::Interact::fInvoke(uint64_t param, uint32_t index) const {
	return pParamCallbacks[index](param);
}

const std::u8string& env::Interact::blockImportModule() const {
	return pBlockImportName;
}
void env::Interact::bindExport(std::u8string_view name) {
	if (pCoreState != LoadingState::none)
		host::Fatal(u8"Cannot bind object [", name, u8"] as export after the core has started loading");
	detail::ProcessAccess::AddCoreBinding(pBlockImportName, std::u8string{ name });
}
uint32_t env::Interact::defineCallback(std::function<void()> fn) {
	pVoidCallbacks.emplace_back(fn);
	return uint32_t(pVoidCallbacks.size() - 1);
}
uint32_t env::Interact::defineCallback(std::function<uint64_t(uint64_t)> fn) {
	pParamCallbacks.emplace_back(fn);
	return uint32_t(pParamCallbacks.size() - 1);
}
void env::Interact::call(const std::u8string& name) const {
	if (pCoreState != LoadingState::loaded)
		host::Fatal(u8"Cannot call core function before the core has been loaded");

	/* check if the function has been exported by the core */
	auto it = pCallVoid.find(name);
	if (it == pCallVoid.end())
		host::Fatal(u8"No void-function named [", name, u8"] is exported by the core");

	/* invoke the function */
	detail::InteractBridge::CallVoid(it->second);
}
uint64_t env::Interact::call(const std::u8string& name, uint64_t param) const {
	if (pCoreState != LoadingState::loaded)
		host::Fatal(u8"Cannot call core function before the core has been loaded");

	/* check if the function has been exported by the core */
	auto it = pCallParam.find(name);
	if (it == pCallParam.end())
		host::Fatal(u8"No param-function named [", name, u8"] is exported by the core");

	/* invoke the function */
	return detail::InteractBridge::CallParam(param, it->second);
}
