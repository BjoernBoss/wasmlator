#include "../env-process.h"

uint32_t env::Mapping::fResolve(env::guest_t address) const {
	host::Debug(str::Format<std::u8string>(u8"Lookup block: [{:#018x}]", address));
	auto it = pMapping.find(address);
	if (it == pMapping.end())
		return 0;
	return it->second;
}

void env::Mapping::fBlockLoaded(bool succeeded) {
	if (!succeeded)
		host::Fatal(u8"Failed loading block");
	host::Debug(u8"Block loading succeeded");

	/* register all exports */
	for (size_t i = 0; i < pExports.size(); ++i) {
		host::Debug(str::Format<std::u8string>(u8"Associating [{}] to [{:#018x}]", pExports[i].name, pExports[i].address));
		uint32_t index = detail::MappingBridge::Define(pExports[i].name.c_str(), pExports[i].name.size(), pExports[i].address);
		if (index == detail::NotFoundIndex)
			host::Fatal(u8"Failed to load [", pExports[i].name, u8"] from block");
		pMapping.insert({ pExports[i].address, index });
	}

	/* mark the loading as succceeded */
	pLoaded();
}
void env::Mapping::fFlushed() {
	host::Debug(u8"Flushed blocks");
	pMapping.clear();
}

void env::Mapping::loadBlock(const uint8_t* data, size_t size, const std::vector<env::BlockExport>& exports, std::function<void()> callback) {
	/* validate the uniqueness of all blocks to be loaded */
	std::unordered_set<env::guest_t> added;
	for (const env::BlockExport& block : exports) {
		if (pMapping.contains(block.address) || added.contains(block.address))
			host::Fatal(str::Format<std::u8string>(u8"Block for [{:#018x}] has already been defined", block.address));
		added.insert(block.address);
	}

	/* try to start the loading (already associate loaded/exports beforehand, as LoadBlock will guarantee that only one block is loaded at a time) */
	host::Debug(u8"Loading block...");
	pLoaded = callback;
	pExports = exports;
	if (!detail::MappingBridge::LoadBlock(data, size, exports.size()))
		host::Fatal(u8"Failed loading block");
}
env::ExecState env::Mapping::execute(env::guest_t address) {
	return static_cast<env::ExecState>(detail::MappingBridge::Execute(address));
}
bool env::Mapping::contains(env::guest_t address) const {
	return (pMapping.find(address) != pMapping.end());
}
void env::Mapping::flush() {
	detail::MappingBridge::Flush();
}
