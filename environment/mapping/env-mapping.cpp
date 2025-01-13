#include "env-mapping.h"

static util::Logger logger{ u8"env::mapping" };

uint32_t env::Mapping::fResolve(env::guest_t address) const {
	/* check if the address has already been translated */
	auto it = pMapping.find(address);
	if (it == pMapping.end()) {
		logger.trace(u8"Lookup block: [", str::As{ U"#018x", address }, u8"] resulted in: None");
		throw env::Translate{ address };
	}

	logger.trace(u8"Lookup block: [", str::As{ U"#018x", address }, u8"] resulted in: [", it->second, u8']');
	return it->second;
}
bool env::Mapping::fCheckLoadable(const std::vector<env::BlockExport>& exports) {
	/* validate the uniqueness of all blocks to be loaded */
	std::unordered_set<env::guest_t> added;
	for (const env::BlockExport& block : exports) {
		if (pMapping.contains(block.address) || added.contains(block.address))
			logger.fatal(u8"Block for [", str::As{ U"#018x", block.address }, u8"] has already been defined");
		added.insert(block.address);
	}

	/* try to reserve the given number of exports */
	return detail::MappingBridge::Reserve(exports.size());
}
void env::Mapping::fBlockExports(const std::vector<env::BlockExport>& exports) {
	/* validate all indices and write them to the map */
	for (size_t i = 0; i < exports.size(); ++i) {
		logger.trace(u8"Associating [", exports[i].name, u8"] to [", str::As{ U"#018x", exports[i].address }, u8"]");
		uint32_t index = detail::MappingBridge::Define(exports[i].name.c_str(), exports[i].name.size(), exports[i].address);
		if (index == detail::InvalidMapping)
			logger.fatal(u8"Failed to load [", exports[i].name, u8"] from block");
		pMapping.insert({ exports[i].address, index });
	}
}

env::guest_t env::Mapping::execute(env::guest_t address) {
	logger.debug(u8"Executing [", str::As{ U"#018x", address }, u8"]");
	return detail::MappingBridge::Execute(address);
}
bool env::Mapping::contains(env::guest_t address) const {
	return (pMapping.find(address) != pMapping.end());
}
void env::Mapping::flush() {
	logger.debug(u8"Flushing blocks");
	pMapping.clear();
	std::memset(pCaches, 0, sizeof(detail::MappingCache) * detail::BlockCacheCount);
}
