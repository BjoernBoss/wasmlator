#include "env-process.h"

env::Process::Process(std::u8string_view name) : pMemory{ this }, pMapping{ this }, pName{ name } {
	str::FormatTo(pLogHeader, u8"[{:>12}]: ", pName);
	pId = 0;
}

env::Process* env::Process::Create(std::u8string_view name, uint32_t caches) {
	env::Process* out = new env::Process{ name };

	/* try to setup the process */
	out->fLog(u8"Creating new process...");
	out->pId = bridge::Process::Create(out);
	if (out->pId == 0) {
		out->fLog(u8"Process creation failed");
		delete out;
		return 0;
	}
	out->fLog(u8"Process created with id [", out->pId, u8']');

	/* initialize the components */
	out->pPhysicalPages = env::PhysPageCount(env::detail::InitAllocBytes);
	uint32_t endOfManagement = 0;
	endOfManagement += detail::MappingAccess{ out }.allocateFromManagement(endOfManagement);
	endOfManagement += detail::MemoryAccess{ out }.configureAndAllocate(endOfManagement, caches, out->pPhysicalPages);
	out->pManagementPages = env::PhysPageCount(endOfManagement);
	return out;
}

void env::Process::fCoreLoaded(bool succeeded) {
	fDebug(u8"Core loaded: [", (succeeded ? u8"succeeded]" : u8"failed]"));
	pLoaded(succeeded);
}
void env::Process::fBlockLoaded(bool succeeded) {
	fDebug(u8"Block loaded: [", (succeeded ? u8"succeeded]" : u8"failed]"));

	/* register all exports and mark the block as loaded (must succeed, as the slots have already been reserved) */
	if (succeeded) {
		for (size_t i = 0; i < pExports.size(); ++i) {
			fDebug(str::Format<std::u8string>(u8"Associating [{}] to [{:#018x}]", pExports[i].name, pExports[i].address));
			bridge::Process::AddExport(pId, pExports[i].name, pExports[i].address);
		}
	}
	pLoaded(succeeded);
}

void env::Process::release() {
	delete this;
}

void env::Process::loadCore(const uint8_t* data, size_t size, std::function<void(bool)> callback) {
	/* protected through glue-code to prevent multiple loads of core modules or parallel loads */
	fDebug(u8"Loading core...");

	/* try to perform the loading and check if it failed (dont set the callback yet, as a load might currently be in progress) */
	if (bridge::Process::LoadCore(pId, data, size))
		pLoaded = callback;
	else {
		fLog(u8"Failed loading core");
		callback(false);
	}
}
void env::Process::loadBlock(const uint8_t* data, size_t size, const std::vector<env::BlockExport>& exports, std::function<void(bool)> callback) {
	/* validate the uniqueness of all blocks to be loaded */
	for (const env::BlockExport& block : exports) {
		if (pMapping.contains(block.address))
			fFail(str::Format<std::u8string>(u8"Block for [{:#018x}] has already been loaded", block.address));
	}

	/* protected through glue-code to prevent parallel loads */
	fDebug(u8"Loading block...");

	/* try to perform the loading and check if it failed (dont set the callback yet, as a load might currently be in progress) */
	if (bridge::Process::LoadBlock(pId, data, size, exports.size())) {
		pLoaded = callback;
		pExports = exports;
	}
	else {
		fLog(u8"Failed loading block");
		callback(false);
	}
}
const std::u8string& env::Process::name() const {
	return pName;
}
env::id_t env::Process::id() const {
	return pId;
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
