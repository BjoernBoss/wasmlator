#include "env-process.h"

#include <wasgen/wasm.h>
#include "../generate/core/gen-core.h"

static env::Process* ProcessInstance = 0;

env::Process* env::Instance() {
	return ProcessInstance;
}

env::Process::Process(std::unique_ptr<sys::Specification>&& specification) : pSpecification{ std::move(specification) } {}

void env::Process::Create(std::unique_ptr<sys::Specification>&& specification) {
	/* try to setup the process */
	host::Log(u8"Creating new process...");
	if (ProcessInstance != 0)
		host::Fatal(u8"Process creation failed as only one process can exist at a time");
	host::Log(u8"Process created");
	ProcessInstance = new env::Process{ std::move(specification) };

	/* initialize the components */
	ProcessInstance->pPhysicalPages = env::PhysPageCount(env::detail::InitAllocBytes);
	uint32_t endOfManagement = 0;
	endOfManagement += detail::ContextAccess{}.configureAndAllocate(endOfManagement);
	endOfManagement += detail::MappingAccess{}.allocateFromManagement(endOfManagement);
	endOfManagement += detail::MemoryAccess{}.configureAndAllocate(endOfManagement, ProcessInstance->pPhysicalPages);
	ProcessInstance->pManagementPages = env::PhysPageCount(endOfManagement);

	/* setup the core module */
	ProcessInstance->fLoadCore();
}

void env::Process::fLoadCore() {
	writer::BinaryWriter writer;
	host::Debug(u8"Loading core...");

	try {
		/* setup the core module to be loaded */
		wasm::Module mod{ &writer };
		pSpecification->setupCore(mod);
	}
	catch (const wasm::Exception& e) {
		host::Fatal(u8"WASM exception occurred while creating the core module: ", e.what());
	}

	/* setup the core loading */
	const std::vector<uint8_t>& data = writer.output();
	if (!detail::ProcessBridge::LoadCore(data.data(), data.size()))
		host::Fatal(u8"Failed loading core");
}
void env::Process::fCoreLoaded(bool succeeded) {
	if (!succeeded)
		host::Fatal(u8"Failed loading core");
	host::Debug(u8"Core loading succeeded");

	/* setup the core-function mappings */
	if (!detail::ProcessBridge::SetupCoreFunctions())
		host::Fatal(u8"Failed to setup core functions");

	/* notify the the specification about the loaded core */
	pSpecification->coreLoaded();
}

void env::Process::release() {
	host::Log(u8"Destroying process...");
	ProcessInstance = 0;
	delete this;
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
