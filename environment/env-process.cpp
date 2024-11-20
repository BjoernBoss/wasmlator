#include "env-process.h"

static env::Process* ProcessInstance = 0;
static uint32_t ProcessId = 0;

env::Process* env::Instance() {
	return ProcessInstance;
}

env::Process::Process(std::unique_ptr<env::System>&& system) : pSystem{ std::move(system) } {}

void env::Process::Create(std::unique_ptr<env::System>&& system) {
	/* try to setup the process */
	host::Log(u8"Creating new process...");
	if (ProcessInstance != 0)
		host::Fatal(u8"Process creation failed as only one process can exist at a time");
	ProcessInstance = new env::Process{ std::move(system) };

	/* initialize the components */
	uintptr_t endOfMemory = 0;
	endOfMemory = std::max(endOfMemory, detail::ContextAccess::Configure());
	endOfMemory = std::max(endOfMemory, detail::MappingAccess::Configure());
	endOfMemory = std::max(endOfMemory, detail::MemoryAccess::Configure(ProcessInstance->pPhysicalPages));
	ProcessInstance->pMemoryPages = detail::PhysPageCount(endOfMemory);

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
		pSystem->setupCore(mod);
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

	/* check if a loading is currently in progress */
	if (pState == State::none)
		host::Fatal(u8"Cannot load a block if core has not been loaded");
	if (pState != State::coreLoaded)
		host::Fatal(u8"Cannot load a block while another load is in progress");
	pState = State::loadingBlock;

	try {
		/* setup the block module to be loaded */
		wasm::Module mod{ &writer };
		pExports = pSystem->setupBlock(mod);
	}
	catch (const wasm::Exception& e) {
		host::Fatal(u8"WASM exception occurred while creating the block module: ", e.what());
	}

	/* check if the exports are valid */
	if (detail::MappingAccess::CheckLoadable(pExports)) {
		/* construct the imports-object for the block */
		detail::ProcessBridge::BlockImportsPrepare();
		for (const auto& [mod, bindings] : pBindings) {
			detail::ProcessBridge::BlockImportsNextMember(mod);
			for (const auto& [name, index] : bindings)
				detail::ProcessBridge::BlockImportsSetValue(name, index);
		}
		detail::ProcessBridge::BlockImportsCommit(false);

		/* try to load the actual block and clear the imports-reference (to allow garbage collection to occur) */
		bool loading = detail::ProcessBridge::LoadBlock(writer.output(), ProcessId);
		detail::ProcessBridge::BlockImportsCommit(true);
		if (loading)
			return;
	}
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
	if (!detail::ProcessBridge::SetupCoreMap())
		host::Fatal(u8"Failed to import all core-functions accessed by the main application");

	/* register all core bindings */
	uint32_t bindingIndex = 0;
	for (auto& [mod, bindings] : pBindings) {
		for (auto& [name, index] : bindings) {
			index = bindingIndex++;
			if (!detail::ProcessBridge::SetExport(name, index))
				host::Fatal(u8"Failed to setup binding for [", name, u8"] of object exported by core to blocks");
		}
	}

	/* notify the core-objects about the core being loaded */
	detail::InteractAccess::CoreLoaded();
	pState = State::coreLoaded;

	/* notify the the system about the loaded core (do not perform
	*	any other operations, as the process might have be destroyed) */
	pSystem->coreLoaded();
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

	/* notify the the system about the loaded core (do not perform
	*	any other operations, as the process might have be destroyed) */
	pSystem->blockLoaded();
	return true;
}
void env::Process::fAddBinding(const std::u8string& mod, const std::u8string& name) {
	pBindings[mod].push_back({ name, 0 });
}

void env::Process::startNewBlock() {
	fLoadBlock();
}
void env::Process::release() {
	host::Log(u8"Destroying process with id [", ProcessId, u8"]...");

	/* reset all mapped core-functions and release the current instance */
	detail::ProcessBridge::ResetCoreMap();
	ProcessInstance = 0;
	delete this;
	host::Log(u8"Process destroyed");
}

const env::System& env::Process::system() const {
	return *pSystem.get();
}
env::System& env::Process::system() {
	return *pSystem.get();
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
