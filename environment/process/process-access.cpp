#include "../environment.h"

uint32_t env::detail::ProcessAccess::PhysicalPages() {
	return env::Instance()->pPhysicalPages;
}
uint32_t env::detail::ProcessAccess::MemoryPages() {
	return env::Instance()->pMemoryPages;
}
void env::detail::ProcessAccess::AddCoreBinding(const std::u8string& mod, const std::u8string& name) {
	env::Instance()->fAddBinding(mod, name);
}
void env::detail::ProcessAccess::LockBindings() {
	env::Instance()->pBindingsClosed = true;
}
size_t env::detail::ProcessAccess::BindingCount() {
	size_t count = 0;
	for (const auto& [mod, bindings] : env::Instance()->pBindings)
		count += bindings.size();
	return count;
}
bool env::detail::ProcessAccess::SingleStep() {
	return env::Instance()->pConfig.singleStep;
}
