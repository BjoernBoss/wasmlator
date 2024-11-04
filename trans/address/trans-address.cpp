#include "trans-address.h"

namespace I = wasm::inst;

trans::detail::Addresses::Addresses(wasm::Module& mod, size_t maxDepth) : pModule{ mod }, pMaxDepth{ maxDepth } {
	pBlockPrototype = pModule.prototype(u8"block_type", {}, { wasm::Type::i64, wasm::Type::i32 });
	pAddresses = pModule.table(u8"linked_addresses", true);
}

trans::detail::Addresses::Placement& trans::detail::Addresses::fPush(env::guest_t address, size_t depth) {
	/* check if the address has already been translated */
	auto it = pTranslated.find(address);
	if (it != pTranslated.end())
		return it->second;
	Placement& entry = pTranslated[address];

	/* check if the address has already been translated in another block */
	if (env::Instance()->mapping().contains(address)) {
		entry.index = pLinks++;
		entry.alreadyExists = true;
		pNeedsStartup = true;
		return entry;
	}

	/* check if the depth-limit has been reached and the address therefore needs to be linked */
	if (depth > pMaxDepth) {
		entry.index = pLinks++;
		return entry;
	}

	/* allocate the address to this module */
	entry.thisModule = true;
	entry.incomplete = true;
	entry.function = pModule.function(str::Format<str::LocalU8<128>>(u8"addr_0x{:#018x}", address), pBlockPrototype, wasm::Export{});
	pQueue.push({ address, depth });
	return entry;
}

trans::detail::PlaceAddress trans::detail::Addresses::pushLocal(env::guest_t address) {
	Placement& entry = fPush(address, pDepth);
	return detail::PlaceAddress{ entry.function, entry.index, entry.thisModule, entry.alreadyExists };
}
void trans::detail::Addresses::pushRoot(env::guest_t address) {
	fPush(address, 0);
}

const wasm::Prototype& trans::detail::Addresses::blockPrototype() {
	return pBlockPrototype;
}
const wasm::Table& trans::detail::Addresses::addresses() {
	return pAddresses;
}
bool trans::detail::Addresses::empty() const {
	return pQueue.empty();
}
trans::detail::OpenAddress trans::detail::Addresses::start() {
	Queued next = pQueue.top();
	pQueue.pop();

	/* mark the block as complete and start it */
	Placement& entry = pTranslated[next.address];
	entry.incomplete = false;
	pDepth = next.depth + 1;
	return { entry.function, next.address };
}
std::vector<env::BlockExport> trans::detail::Addresses::close(const detail::MappingState& mappingState) {
	/* setup the addresses-table limit */
	pModule.limit(pAddresses, wasm::Limit{ uint32_t(pLinks), uint32_t(pLinks) });

	/* ensure that all functions have been written and collect the exports */
	std::vector<env::BlockExport> exports;
	for (const auto& [addr, place] : pTranslated) {
		if (!place.thisModule)
			continue;
		std::u8string name{ place.function.id() };
		if (place.incomplete)
			host::Fail(u8"Block Address [", name, u8"] has not been produced");
		exports.push_back({ name, addr });
	}

	/* setup the startup-function for the already existing imports */
	if (pNeedsStartup) {
		wasm::Function startup = pModule.function(u8"_setup_imports", {}, {});
		wasm::Sink sink{ startup };
		detail::MappingWriter mapping{ mappingState, sink };

		/* iterate over the functions and write them all to the imports */
		for (const auto& [addr, place] : pTranslated) {
			if (place.thisModule || !place.alreadyExists)
				continue;

			/* perform the lookup (no need to verify index, as the function must exist) */
			sink[I::U32::Const(place.index)];
			sink[I::U64::Const(addr)];
			mapping.makeLookup();
			mapping.makeLoadFunction();
			sink[I::Table::Set(pAddresses)];
		}

		pModule.startup(startup);
	}
	return exports;
}
