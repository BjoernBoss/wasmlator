#include "../environment.h"

bool env::detail::ProcessAccess::Setup(env::Process& process, std::unique_ptr<env::System>&& system, uint32_t pageSize, uint32_t memoryCaches, uint32_t contextSize, bool logBlocks) {
	return process.fSetup(std::move(system), pageSize, memoryCaches, contextSize, logBlocks);
}
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
void env::detail::ProcessAccess::HandleTask(const std::u8string& task, std::function<void(std::u8string_view)> callback) {
	env::Instance()->fHandleTask(task, callback);
}
