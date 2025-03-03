/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025 Bjoern Boss Henrichsen */
#include "env-mapping.h"

static util::Logger logger{ u8"env::mapping" };

void env::Mapping::fFlush() {
	logger.debug(u8"Flushing blocks");

	/* clear the mapping and cache */
	pMapping.clear();
	std::memset(pFastCache, 0, sizeof(detail::MappingCache) * detail::BlockFastCount);
	pTotalBlockCount = 0;

	/* clear the actual reference to all blocks (to allow the garbage collection to run) */
	detail::MappingBridge::Flush();
}
uint32_t env::Mapping::fResolve(env::guest_t address) {
	/* fast-cache lookup, check if the address is in the cache (index will be zero, if its a miss) */
	uint32_t index = uint32_t((address >> detail::BlockFastCacheBits) ^ address) & (detail::BlockFastCount - 1);
	if (pFastCache[index].address == address && pFastCache[index].index != detail::InvalidMapping)
		return pFastCache[index].index;

	/* check if the address has already been translated */
	auto it = pMapping.find(address);
	if (it == pMapping.end()) {
		logger.trace(u8"Lookup block: [", str::As{ U"#018x", address }, u8"] resulted in: None");
		throw env::Translate{ address };
	}
	logger.trace(u8"Lookup block: [", str::As{ U"#018x", address }, u8"] resulted in: [", it->second, u8']');

	/* write the result to the cache and return the index */
	pFastCache[index] = { address, it->second };
	return pFastCache[index].index;
}
void env::Mapping::fCheckLoadable(const std::vector<env::BlockExport>& exports) {
	/* validate the uniqueness of all blocks to be loaded */
	std::unordered_set<env::guest_t> added;
	for (const env::BlockExport& block : exports) {
		if (pMapping.contains(block.address) || added.contains(block.address))
			logger.fatal(u8"Block for [", str::As{ U"#018x", block.address }, u8"] has already been defined");
		added.insert(block.address);
	}

	/* try to reserve the given number of exports */
	if (!detail::MappingBridge::Reserve(exports.size()))
		logger.fatal(u8"Unabled to reserve [", exports.size(), u8"] slots for block exports");
}
void env::Mapping::fBlockExports(const std::vector<env::BlockExport>& exports) {
	++pTotalBlockCount;

	/* validate all indices and write them to the map */
	for (size_t i = 0; i < exports.size(); ++i) {
		/* resolve the mapping from the loaded block to the core function-mapping table */
		uint32_t index = detail::MappingBridge::Define(exports[i].name.c_str(), exports[i].name.size(), exports[i].address);
		if (index == detail::InvalidMapping)
			logger.fatal(u8"Failed to load [", exports[i].name, u8"] from block");

		/* associate the acutal function address to the given index */
		logger.trace(u8"Associating [", exports[i].name, u8"] to [", str::As{ U"#018x", exports[i].address }, u8"] and index [", index, u8']');
		pMapping.insert({ exports[i].address, index });
	}

	/* log the new statistics */
	logger.info(u8"Total blocks loaded: ", pTotalBlockCount, u8" | Total super-blocks translated: ", pMapping.size());
}

void env::Mapping::execute(env::guest_t address) {
	logger.debug(u8"Executing [", str::As{ U"#018x", address }, u8']');
	detail::MappingBridge::Execute(address);
}
bool env::Mapping::contains(env::guest_t address) const {
	return (pMapping.find(address) != pMapping.end());
}
void env::Mapping::flush() {
	fFlush();
}
