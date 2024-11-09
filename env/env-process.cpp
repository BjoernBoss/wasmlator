#include "env-process.h"

static env::Process* ProcessInstance = 0;

env::Process* env::Instance() {
	return ProcessInstance;
}

void env::Process::Create(uint32_t caches, uint32_t context) {
	/* try to setup the process */
	host::Log(u8"Creating new process...");
	if (ProcessInstance != 0)
		host::Fail(u8"Process creation failed as only one process can exist at a time");
	host::Log(u8"Process created");
	ProcessInstance = new env::Process{};

	/* initialize the components */
	ProcessInstance->pPhysicalPages = env::PhysPageCount(env::detail::InitAllocBytes);
	uint32_t endOfManagement = 0;
	endOfManagement += detail::ContextAccess{}.configureAndAllocate(endOfManagement, context);
	endOfManagement += detail::MappingAccess{}.allocateFromManagement(endOfManagement);
	endOfManagement += detail::MemoryAccess{}.configureAndAllocate(endOfManagement, caches, ProcessInstance->pPhysicalPages);
	ProcessInstance->pManagementPages = env::PhysPageCount(endOfManagement);
}

void env::Process::fCoreLoaded(bool succeeded) {
	if (!succeeded)
		host::Fail(u8"Failed loading core");
	host::Debug(u8"Core loading succeeded");

	/* setup the core-function mappings */
	if (!detail::ProcessBridge::SetupCoreFunctions())
		host::Fail(u8"Failed to setup core functions");

	/* notify the the environment about the loaded core */
	pLoaded();
}

void env::Process::loadCore(const uint8_t* data, size_t size, std::function<void()> callback) {
	/* immediately set the callback as LoadCore will fail if a core is already being loaded */
	pLoaded = callback;
	host::Debug(u8"Loading core...");
	if (!detail::ProcessBridge::LoadCore(data, size))
		host::Fail(u8"Failed loading core");
}
void env::Process::release() {
	host::Log(u8"Destroying process...");
	ProcessInstance = 0;
	delete this;
}

const env::Context& env::Process::context() const {
	return pContext;
}
env::Context& env::Process::context() {
	return pContext;
}
const env::Memory& env::Process::memory() const {
	return pMemory;
}
env::Memory& env::Process::memory() {
	return pMemory;
}
const env::Mapping& env::Process::mapping() const {
	return pMapping;
}
env::Mapping& env::Process::mapping() {
	return pMapping;
}
