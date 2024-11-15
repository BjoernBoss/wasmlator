#include "env-process.h"

#include <wasgen/wasm.h>

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

	pState = State::loadingCore;
	try {
		/* setup the core module to be loaded */
		wasm::Module mod{ &writer };
		pSpecification->setupCore(mod);
	}
	catch (const wasm::Exception& e) {
		host::Fatal(u8"WASM exception occurred while creating the core module: ", e.what());
	}

	/* setup the core loading */
	if (!detail::ProcessBridge::LoadCore(writer.output(), ProcessId))
		host::Fatal(u8"Failed initiating loading of core for [", ProcessId, u8']');
}
void env::Process::fLoadBlock() {
	host::Debug(u8"Loading block for [", ProcessId, u8"]...");
	writer::BinaryWriter writer;
	std::vector<env::BlockExport> exports;

	/* check if a loading is currently in progress */
	if (pState == State::none)
		host::Fatal(u8"Cannot load a block if core has not been loaded");
	if (pState != State::coreLoaded)
		host::Fatal(u8"Cannot load a block while another load is in progress");
	pState = State::loadingBlock;

	try {
		/* setup the block module to be loaded */
		wasm::Module mod{ &writer };
		exports = pSpecification->setupBlock(mod);
	}
	catch (const wasm::Exception& e) {
		host::Fatal(u8"WASM exception occurred while creating the core module: ", e.what());
	}

	/* check if the exports are valid and start the loading */
	pExports = exports;
	if (!detail::MappingAccess::CheckLoadable(pExports) || !detail::ProcessBridge::LoadBlock(writer.output(), ProcessId))
		host::Fatal(u8"Failed initiating loading of core for [", ProcessId, u8']');
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
	if (!detail::ProcessBridge::SetupCoreMap())
		host::Fatal(u8"Failed to import all core-functions accessed by the main application");

	/* register all core bindings */
	for (size_t i = 0; i < pBindings.size(); ++i) {
		if (!detail::ProcessBridge::SetExport(pBindings[i].name, uint32_t(i)))
			host::Fatal(u8"Failed to setup binding for [", pBindings[i].name, u8"] of object exported by core to blocks");
	}

	/* notify the core-objects about the core being loaded */
	detail::InteractAccess::CoreLoaded();
	pState = State::coreLoaded;

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
	pState = State::coreLoaded;
	pExports.clear();

	/* notify the the specification about the loaded core */
	pSpecification->blockLoaded();
	return true;
}
void env::Process::fAddBinding(const std::u8string& mod, const std::u8string& name) {
	pBindings.push_back({ mod, name });
}

void env::Process::nextBlock() {
	fLoadBlock();
}
void env::Process::release() {
	host::Log(u8"Destroying process with id [", ProcessId, u8"]...");

	/* reset all mapped core-functions and release the current instance */
	detail::ProcessBridge::Reset();
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
