#include "env-process.h"

#include <wasgen/wasm.h>
#include "../generate/interact/interact-builder.h"

static env::Process* ProcessInstance = 0;
static uint32_t ProcessId = 0;

env::Process* env::Instance() {
	return ProcessInstance;
}

env::Process::Process(std::unique_ptr<sys::Specification>&& specification) : pSpecification{ std::move(specification) } {}

void env::Process::Create(std::unique_ptr<sys::Specification>&& specification) {
	/* try to setup the process */
	host::Log(u8"Creating new process...");
	if (ProcessInstance != 0)
		host::Fatal(u8"Process creation failed as only one process can exist at a time");
	ProcessInstance = new env::Process{ std::move(specification) };

	/* initialize the components */
	ProcessInstance->pPhysicalPages = env::PhysPageCount(env::detail::InitAllocBytes);
	uint32_t endOfManagement = 0;
	endOfManagement += detail::ContextAccess::ConfigureAndAllocate(endOfManagement);
	endOfManagement += detail::MappingAccess::AllocateFromManagement(endOfManagement);
	endOfManagement += detail::MemoryAccess::ConfigureAndAllocate(endOfManagement, ProcessInstance->pPhysicalPages);
	ProcessInstance->pManagementPages = env::PhysPageCount(endOfManagement);

	/* allocate the next process-id */
	host::Log(u8"Process created with id [", ++ProcessId, u8"]");

	/* setup the core module */
	ProcessInstance->fLoadCore();
}

void env::Process::fLoadCore() {
	host::Debug(u8"Loading core for [", ProcessId, u8"]...");
	writer::BinaryWriter writer;

	pLoading = LoadingState::core;
	try {
		/* setup the core module to be loaded */
		wasm::Module mod{ &writer };
		pSpecification->setupCore(mod);

		/* finalize the internal components */
		gen::detail::InteractBuilder{}.finalizeCoreBody(mod);
	}
	catch (const wasm::Exception& e) {
		host::Fatal(u8"WASM exception occurred while creating the core module: ", e.what());
	}

	/* setup the core loading */
	const std::vector<uint8_t>& data = writer.output();
	if (!detail::ProcessBridge::LoadCore(data.data(), data.size(), ProcessId))
		host::Fatal(u8"Failed initiating loading of core for [", ProcessId, u8']');
}
void env::Process::fLoadBlock() {
	host::Debug(u8"Loading block for [", ProcessId, u8"]...");
	writer::BinaryWriter writer;
	std::vector<env::BlockExport> exports;

	/* check if a loading is currently in progress */
	if (pLoading != LoadingState::none)
		host::Fatal(u8"Cannot load a block while another load is in progress");
	pLoading = LoadingState::block;

	try {
		/* setup the block module to be loaded */
		wasm::Module mod{ &writer };
		exports = pSpecification->setupBlock(mod);
	}
	catch (const wasm::Exception& e) {
		host::Fatal(u8"WASM exception occurred while creating the core module: ", e.what());
	}

	/* setup the block loading */
	pExports = exports;
	if (!detail::MappingAccess::LoadBlock(writer.output(), pExports, ProcessId))
		host::Fatal(u8"Failed initiating loading of block for [", ProcessId, u8']');
}
bool env::Process::fCoreLoaded(uint32_t process, bool succeeded) {
	/* check if the ids still match and otherwise simply discard the call (no need to update
	*	the loading-state, as it will not be affected by a process of a different id) */
	if (process != ProcessId)
		return false;
	if (!succeeded)
		host::Fatal(u8"Failed loading core for [", ProcessId, u8']');
	host::Debug(u8"Core loading succeeded for [", ProcessId, u8']');

	/* setup the core-function mappings */
	if (!detail::ProcessBridge::SetupCoreFunctions())
		host::Fatal(u8"Failed to import all core-functions accessed by the main application");

	/* notify the core-objects about the core being loaded */
	detail::InteractAccess::CoreLoaded();
	detail::MemoryAccess::CoreLoaded();
	detail::MappingAccess::CoreLoaded();
	pLoading = LoadingState::none;

	/* notify the the specification about the loaded core */
	pSpecification->coreLoaded();
	return true;

}
bool env::Process::fBlockLoaded(uint32_t process, bool succeeded) {
	/* check if the ids still match and otherwise simply discard the call (no need to update
	*	the loading-state, as it will not be affected by a process of a different id) */
	if (process != ProcessId)
		return false;
	if (!succeeded)
		host::Fatal(u8"Failed loading block for [", ProcessId, u8']');
	host::Debug(u8"Block loading succeeded for [", ProcessId, u8']');

	/* register all exports and update the loading-state */
	detail::MappingAccess::BlockLoaded(pExports);
	pLoading = LoadingState::none;
	pExports.clear();

	/* notify the the specification about the loaded core */
	pSpecification->blockLoaded();
	return true;
}

void env::Process::nextBlock() {
	fLoadBlock();
}
void env::Process::release() {
	host::Log(u8"Destroying process with id [", ProcessId, u8"]...");
	ProcessInstance = 0;
	delete this;
	host::Log(u8"Process destroyed");
}

const sys::Specification& env::Process::specification() const {
	return *pSpecification.get();
}
sys::Specification& env::Process::specification() {
	return *pSpecification.get();
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
const env::Interact& env::Process::interact() const {
	return pInteract;
}
env::Interact& env::Process::interact() {
	return pInteract;
}
