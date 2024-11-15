#include "../env-process.h"

uint32_t env::Mapping::fResolve(env::guest_t address) const {
	host::Debug(str::Format<std::u8string>(u8"Lookup block: [{:#018x}]", address));
	auto it = pMapping.find(address);
	if (it == pMapping.end())
		return 0;
	return it->second;
}
void env::Mapping::fFlushed() {
	host::Debug(u8"Flushed blocks");
	pMapping.clear();
}
bool env::Mapping::fCheckLoadable(const std::vector<env::BlockExport>& exports) {
	/* validate the uniqueness of all blocks to be loaded */
	std::unordered_set<env::guest_t> added;
	for (const env::BlockExport& block : exports) {
		if (pMapping.contains(block.address) || added.contains(block.address))
			host::Fatal(str::Format<std::u8string>(u8"Block for [{:#018x}] has already been defined", block.address));
		added.insert(block.address);
	}

	/* try to reserve the given number of exports */
	return detail::MappingBridge::Reserve(exports.size());
}
void env::Mapping::fBlockExports(const std::vector<env::BlockExport>& exports) {
	/* validate all indices and write them to the map */
	for (size_t i = 0; i < exports.size(); ++i) {
		host::Debug(str::Format<std::u8string>(u8"Associating [{}] to [{:#018x}]", exports[i].name, exports[i].address));
		uint32_t index = detail::MappingBridge::Define(exports[i].name.c_str(), exports[i].name.size(), exports[i].address);
		if (index == detail::NotFoundIndex)
			host::Fatal(u8"Failed to load [", exports[i].name, u8"] from block");
		pMapping.insert({ exports[i].address, index });
	}
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
