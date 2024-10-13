#include "../env-process.h"

env::Context::Context(std::u8string_view name, env::Process* process) : pProcess{ process }, pName{ name }, pSelfName{ u8"core" } {
	str::FormatTo(pLogHeader, u8"[{:>12}]: ", pName);
	pId = 0;
}
env::Context::~Context() {
	if (pId != 0) {
		pProcess->log(u8"Destroying context [", pId, u8']');
		bridge::Context::Destroy(pId);
	}
}

bool env::Context::fCreate(std::function<void(env::guest_t)> translate) {
	pProcess->log(u8"Creating new context...");
	pTranslate = translate;

	/* try to create the new context */
	pId = bridge::Context::Create(pProcess);
	if (pId == 0) {
		pProcess->log(u8"Context creation failed");
		return false;
	}
	pProcess->log(u8"Context created with id [", pId, u8']');
	return true;
}
void env::Context::fCoreLoaded(bool succeeded) {
	pProcess->debug(u8"Core loaded: [", (succeeded ? u8"succeeded]" : u8"failed]"));
	pLoaded(succeeded);
}
void env::Context::fBlockLoaded(bool succeeded) {
	pProcess->debug(u8"Block loaded: [", (succeeded ? u8"succeeded]" : u8"failed]"));

	/* register all exports and mark the block as loaded (must succeed, as the slots have already been reserved) */
	if (succeeded) {
		for (size_t i = 0; i < pExports.size(); ++i) {
			pProcess->debug(str::Format<std::u8string>(u8"Associating [{}] to [{:#018x}]", pExports[i].name, pExports[i].address));
			bridge::Context::AddExport(pId, pExports[i].name, pExports[i].address);
		}
	}
	pLoaded(succeeded);
}
void env::Context::fTranslate(env::guest_t address) {
	pProcess->debug(str::Format<std::u8string>(u8"Translate: [{:#018x}]", address));
	pTranslate(address);
}

void env::Context::loadCore(const uint8_t* data, size_t size, std::function<void(bool)> callback) {
	/* protected through glue-code to prevent multiple loads of core modules or parallel loads */
	pProcess->debug(u8"Loading core...");

	/* try to perform the loading and check if it failed (dont set the callback yet, as a load might currently be in progress) */
	if (bridge::Context::LoadCore(pId, data, size))
		pLoaded = callback;
	else {
		pProcess->log(u8"Failed loading core");
		callback(false);
	}
}
void env::Context::loadBlock(const uint8_t* data, size_t size, const std::vector<env::BlockExport>& exports, std::function<void(bool)> callback) {
	/* protected through glue-code to prevent parallel loads */
	pProcess->debug(u8"Loading block...");

	/* try to perform the loading and check if it failed (dont set the callback yet, as a load might currently be in progress) */
	if (bridge::Context::LoadBlock(pId, data, size, exports.size())) {
		pLoaded = callback;
		pExports = exports;
	}
	else {
		pProcess->log(u8"Failed loading block");
		callback(false);
	}
}
const std::u8string& env::Context::name() const {
	return pName;
}
const std::u8string& env::Context::logHeader() const {
	return pLogHeader;
}
const std::u8string& env::Context::selfName() const {
	return pSelfName;
}
wasm::Import env::Context::imported() const {
	return wasm::Import{ pSelfName };
}
env::id_t env::Context::id() const {
	return pId;
}
