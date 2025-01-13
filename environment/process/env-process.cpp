#include "../environment.h"

static host::Logger logger{ u8"env::process" };

namespace global {
	static std::unique_ptr<env::Process> Instance;
	static uint32_t ProcId = 0;
}

env::Process* env::Instance() {
	return global::Instance.get();
}
bool env::SetInstance(std::unique_ptr<env::System>&& system, uint32_t pageSize, uint32_t memoryCaches, uint32_t contextSize, bool logBlocks) {
	if (global::Instance.get() != 0) {
		logger.error(u8"Cannot create process as only one process can exist at a time");
		return false;
	}

	/* allocate the new instance */
	logger.log(u8"Creating new process...");
	global::Instance = std::make_unique<env::Process>();

	/* configure the instance */
	if (detail::ProcessAccess::Setup(*global::Instance.get(), std::move(system), pageSize, memoryCaches, contextSize, logBlocks)) {
		logger.log(u8"Process created");
		return true;
	}
	global::Instance.reset();
	return false;
}
void env::ClearInstance() {
	logger.log(u8"Destroying process with id [", global::ProcId, u8"]...");

	/* reset all mapped core-functions and release the current instance */
	detail::ProcessBridge::ResetCoreMap();
	global::Instance.reset();
	logger.log(u8"Process destroyed");
}


bool env::Process::fSetup(std::unique_ptr<env::System>&& system, uint32_t pageSize, uint32_t memoryCaches, uint32_t contextSize, bool logBlocks) {
	/* apply the configuration */
	pSystem = std::move(system);
	pPageSize = pageSize;
	pMemoryCaches = memoryCaches;
	pContextSize = contextSize;
	pLogBlocks = logBlocks;

	/* validate the configuration */
	if (pPageSize == 0 || ((pPageSize - 1) & pPageSize) != 0) {
		logger.error(u8"PageSize must be greater than zero and a power of two");
		return false;
	}

	/* log the generation-configuration */
	logger.info(u8"  Page Size    : ", str::As{ U"#x", pPageSize });
	logger.info(u8"  Memory Caches: ", pMemoryCaches);
	logger.info(u8"  Context Size : ", pContextSize);
	logger.info(u8"  Log Blocks   : ", str::As{ U"S", pLogBlocks });

	/* initialize the components */
	uintptr_t endOfMemory = 0;
	endOfMemory = std::max(endOfMemory, detail::ContextAccess::Configure());
	endOfMemory = std::max(endOfMemory, detail::MappingAccess::Configure());
	auto memoryConfig = detail::MemoryAccess::Configure(pPhysicalPages);
	if (!memoryConfig.has_value())
		return false;
	pMemoryPages = detail::PhysPageCount(std::max(endOfMemory, memoryConfig.value()));

	/* allocate the next process-id */
	logger.log(u8"Process created with id [", ++global::ProcId, u8"]");

	/* setup the core module */
	return fLoadCore();
}
void env::Process::fAddBinding(const std::u8string& mod, const std::u8string& name) {
	if (pBindingsClosed)
		logger.fatal(u8"Cannot bind object [", name, u8"] as export after the core has started loading");
	pBindings[mod].push_back({ name, 0 });
}
void env::Process::fHandleTask(const std::u8string& task, std::function<void(std::u8string_view)> callback) {
	/* check if a task is currently active, in which case no other task can be processed */
	if (pTaskActive)
		logger.fatal(u8"Can only process one task at a time");
	logger.debug(u8"Handling task [", task, u8"] for [", global::ProcId, u8"]...");

	/* setup the task and pass it out (can be handled in-place, in which case the callback will be called immediately again) */
	pTaskActive = true;
	pTaskCallback = callback;
	detail::ProcessBridge::HandleTask(task, global::ProcId);
}
bool env::Process::fTaskCompleted(uint32_t process, std::u8string_view response) {
	/* check if the ids still match and otherwise simply discard the call (will not affect any internal states) */
	if (process != global::ProcId)
		return false;
	logger.debug(u8"Task sucessfully completed for [", global::ProcId, u8"] with response: [", response.substr(0, 20), (response.size() > 20 ? u8"...]" : u8"]"));

	/* invoke the task-callback */
	std::function<void(std::u8string_view)> callback = pTaskCallback;
	pTaskActive = false;
	callback(response);
	return true;
}

bool env::Process::fLoadCore() {
	logger.debug(u8"Loading core for [", global::ProcId, u8"]...");
	wasm::BinaryWriter binOutput;

	pState = State::loadingCore;
	try {
		/* setup the core module to be loaded */
		wasm::Module mod{ &binOutput };
		if (!pSystem->setupCore(mod))
			return false;
		mod.close();
	}
	catch (const wasm::Exception& e) {
		logger.error(u8"WASM exception occurred while creating the core module: ", e.what());
		return false;
	}

	/* setup the core loading task */
	const std::vector<uint8_t>& data = binOutput.output();
	fHandleTask(str::u8::Build(u8"core:0x", static_cast<const void*>(data.data()), u8':', str::As{ U"#x", data.size() }), [this](std::u8string_view) {
		fCoreLoaded();
		});
	return true;
}
void env::Process::fCoreLoaded() {
	logger.debug(u8"Core loading succeeded for [", global::ProcId, u8']');

	/* setup the core-function mappings */
	if (!detail::ProcessBridge::SetupCoreMap()) {
		logger.error(u8"Failed to import all core-functions accessed by the main application");
		pSystem->shutdown();
		return;
	}

	/* register all core bindings */
	uint32_t bindingIndex = 0;
	for (auto& [mod, bindings] : pBindings) {
		for (auto& [name, index] : bindings) {
			index = bindingIndex++;
			if (detail::ProcessBridge::SetExport(name, index))
				continue;

			/* log the error and shutdown the created system */
			logger.error(u8"Failed to setup binding for [", name, u8"] of object exported by core to blocks");
			pSystem->shutdown();
			return;
		}
	}

	/* notify the core-objects about the core being loaded */
	detail::InteractAccess::CoreLoaded();
	pState = State::coreLoaded;

	/* notify the the system about the loaded core and check if the setup failed */
	if (!pSystem->coreLoaded())
		pSystem->shutdown();
}
void env::Process::fLoadBlock() {
	logger.debug(u8"Loading block for [", global::ProcId, u8"]...");
	wasm::BinaryWriter binOutput;

	/* check if a loading is currently in progress */
	if (pState == State::none)
		logger.fatal(u8"Cannot load a block if core has not been loaded");
	if (pState != State::coreLoaded)
		logger.fatal(u8"Cannot load a block while another load is in progress");
	pState = State::loadingBlock;

	try {
		/* setup the writer to either produce only the binary output, or the binary and text output */
		std::unique_ptr<wasm::TextWriter> tWriter = (pLogBlocks ? std::make_unique<wasm::TextWriter>(u8"  ") : 0);
		wasm::SplitWriter writer = wasm::SplitWriter{ &binOutput, tWriter.get() };

		/* setup the block module to be loaded */
		wasm::Module mod{ &writer };
		pExports = pSystem->setupBlock(mod);
		mod.close();

		/* check if the text-output should be logged */
		if (pLogBlocks)
			logger.trace(L'\n', tWriter->output());
	}
	catch (const wasm::Exception& e) {
		logger.fatal(u8"WASM exception occurred while creating the block module: ", e.what());
	}

	/* check if the exports are valid */
	if (!detail::MappingAccess::CheckLoadable(pExports))
		logger.fatal(u8"Failed initiating loading of block for [", global::ProcId, u8']');

	/* construct the imports-object for the block */
	detail::ProcessBridge::BlockImportsPrepare();
	for (const auto& [mod, bindings] : pBindings) {
		detail::ProcessBridge::BlockImportsNextMember(mod);
		for (const auto& [name, index] : bindings)
			detail::ProcessBridge::BlockImportsSetValue(name, index);
	}
	detail::ProcessBridge::BlockImportsCommit(false);

	/* setup the block loading task */
	const std::vector<uint8_t>& data = binOutput.output();
	fHandleTask(str::u8::Build(u8"block:0x", static_cast<const void*>(data.data()), u8':', str::As{ U"#x", data.size() }), [this](std::u8string_view) {
		fBlockLoaded();
		});

	/* clear the imports-reference (to allow garbage collection to occur) */
	detail::ProcessBridge::BlockImportsCommit(true);
}
void env::Process::fBlockLoaded() {
	logger.debug(u8"Block loading succeeded for [", global::ProcId, u8']');

	/* register all exports and update the loading-state */
	detail::MappingAccess::BlockLoaded(pExports);
	pState = State::coreLoaded;
	pExports.clear();

	/* notify the the system about the loaded core (do not perform
	*	any other operations, as the process might have be destroyed) */
	pSystem->blockLoaded();
}

const std::u8string& env::Process::blockImportModule() const {
	return pBlockImportName;
}
void env::Process::bindExport(std::u8string_view name) {
	fAddBinding(pBlockImportName, std::u8string{ name });
}
void env::Process::startNewBlock() {
	fLoadBlock();
}

void env::Process::shutdown() {
	pSystem->shutdown();
}
const env::Context& env::Process::context() const {
	return pContext;
}
env::Context& env::Process::context() {
	return pContext;
}
const env::FileSystem& env::Process::filesystem() const {
	return pFileSystem;
}
env::FileSystem& env::Process::filesystem() {
	return pFileSystem;
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

uint32_t env::Process::getId() const {
	return global::ProcId;
}
uint32_t env::Process::pageSize() const {
	return pPageSize;
}
uint32_t env::Process::memoryCaches() const {
	return pMemoryCaches;
}
uint32_t env::Process::contextSize() const {
	return pContextSize;
}
bool env::Process::logBlocks() const {
	return pLogBlocks;
}
