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
	if (!detail::ProcessBridge::Create())
		host::Fail(u8"Process creation failed");
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

	/* register all exports and mark the core as loaded (must succeed, as the slots have already been reserved) */
	for (size_t i = 0; i < pExports.size(); ++i) {
		host::Debug(str::Format<std::u8string>(u8"Associating [{}] to [{:#018x}]", pExports[i].name, pExports[i].address));
		detail::ProcessBridge::AddCoreExport(pExports[i].name, pExports[i].address);
	}
	pLoaded();
}
void env::Process::fBlockLoaded(bool succeeded) {
	if (!succeeded)
		host::Fail(u8"Failed loading block");
	host::Debug(u8"Block loading succeeded");

	/* register all exports and mark the block as loaded (must succeed, as the slots have already been reserved) */
	for (size_t i = 0; i < pExports.size(); ++i) {
		host::Debug(str::Format<std::u8string>(u8"Associating [{}] to [{:#018x}]", pExports[i].name, pExports[i].address));
		detail::ProcessBridge::AddBlockExport(pExports[i].name, pExports[i].address);
	}
	pLoaded();
}

void env::Process::release() {
	host::Log(u8"Destroying process...");
	detail::ProcessBridge::Destroy();
	host::Log(u8"Process destroyed");
	delete this;
}

void env::Process::loadCore(const uint8_t* data, size_t size, const std::vector<env::BlockExport>& exports, std::function<void()> callback) {
	/* validate the uniqueness of all blocks to be loaded */
	std::unordered_set<env::guest_t> added;
	for (const env::BlockExport& block : exports) {
		if (pMapping.contains(block.address) || added.contains(block.address))
			host::Fail(str::Format<std::u8string>(u8"Block for [{:#018x}] has already been defined", block.address));
		added.insert(block.address);
	}

	/* try to perform the loading and check if it failed (dont set the callback yet, as a load might currently be in progress) */
	host::Debug(u8"Loading core...");
	if (!detail::ProcessBridge::LoadCore(data, size, exports.size()))
		host::Fail(u8"Failed loading core");
	pLoaded = callback;
	pExports = exports;
}
void env::Process::loadBlock(const uint8_t* data, size_t size, const std::vector<env::BlockExport>& exports, std::function<void()> callback) {
	/* validate the uniqueness of all blocks to be loaded */
	std::unordered_set<env::guest_t> added;
	for (const env::BlockExport& block : exports) {
		if (pMapping.contains(block.address) || added.contains(block.address))
			host::Fail(str::Format<std::u8string>(u8"Block for [{:#018x}] has already been defined", block.address));
		added.insert(block.address);
	}

	/* try to perform the loading and check if it failed (dont set the callback yet, as a load might currently be in progress) */
	host::Debug(u8"Loading block...");
	if (!detail::ProcessBridge::LoadBlock(data, size, exports.size()))
		host::Fail(u8"Failed loading block");
	pLoaded = callback;
	pExports = exports;
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
