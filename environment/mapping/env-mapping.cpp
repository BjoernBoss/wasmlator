#include "env-mapping.h"

uint32_t env::Mapping::fResolve(env::guest_t address) const {
	/* check if the address has already been translated */
	auto it = pMapping.find(address);
	if (it == pMapping.end()) {
		host::Debug(str::u8::Format(u8"Lookup block: [{:#018x}] resulted in: None", address));
		throw env::Translate{ address };
	}

	host::Debug(str::u8::Format(u8"Lookup block: [{:#018x}] resulted in: [{}]", address, it->second));
	return it->second;
}
bool env::Mapping::fCheckLoadable(const std::vector<env::BlockExport>& exports) {
	/* validate the uniqueness of all blocks to be loaded */
	std::unordered_set<env::guest_t> added;
	for (const env::BlockExport& block : exports) {
		if (pMapping.contains(block.address) || added.contains(block.address))
			host::Fatal(str::u8::Format(u8"Block for [{:#018x}] has already been defined", block.address));
		added.insert(block.address);
	}

	/* try to reserve the given number of exports */
	return detail::MappingBridge::Reserve(exports.size());
}
void env::Mapping::fBlockExports(const std::vector<env::BlockExport>& exports) {
	/* validate all indices and write them to the map */
	for (size_t i = 0; i < exports.size(); ++i) {
		host::Debug(str::u8::Format(u8"Associating [{}] to [{:#018x}]", exports[i].name, exports[i].address));
		uint32_t index = detail::MappingBridge::Define(exports[i].name.c_str(), exports[i].name.size(), exports[i].address);
		if (index == detail::InvalidMapping)
			host::Fatal(u8"Failed to load [", exports[i].name, u8"] from block");
		pMapping.insert({ exports[i].address, index });
	}
}

void env::Mapping::execute(env::guest_t address) {
	host::Debug(str::u8::Format(u8"Executing [{:#018x}]", address));
	detail::MappingBridge::Execute(address);
}
bool env::Mapping::contains(env::guest_t address) const {
	return (pMapping.find(address) != pMapping.end());
}
void env::Mapping::flush() {
	host::Debug(u8"Flushing blocks");
	pMapping.clear();
	std::memset(pCaches, 0, sizeof(detail::MappingCache) * detail::BlockCacheCount);
}
