#include "../env-process.h"

uint32_t env::detail::ProcessAccess::PhysicalPages() {
	return env::Instance()->pPhysicalPages;
}
uint32_t env::detail::ProcessAccess::ManagementPages() {
	return env::Instance()->pManagementPages;
}
void env::detail::ProcessAccess::AddCoreBinding(const std::u8string& mod, const std::u8string& name) {
	env::Instance()->fAddBinding(mod, name);
}
size_t env::detail::ProcessAccess::BindingCount() {
	return env::Instance()->pBindings.size();
}
