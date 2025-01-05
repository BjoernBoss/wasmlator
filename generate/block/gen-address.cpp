#include "../environment/environment.h"
#include "../generate.h"

static host::Logger logger{ u8"gen::block" };

gen::detail::Addresses::Placement& gen::detail::Addresses::fPush(env::guest_t address, size_t depth) {
	/* check if the address has already been translated */
	auto it = pTranslated.find(address);
	if (it != pTranslated.end())
		return it->second;
	Placement& entry = pTranslated[address];

	/* check if the address has already been translated in another block */
	if (env::Instance()->mapping().contains(address)) {
		entry.alreadyExists = true;
		pNeedsStartup = true;
	}

	/* check if the depth-limit has been reached or the entry already exists and setup the address-table required for linking */
	if (entry.alreadyExists || depth > gen::Instance()->translationDepth()) {
		entry.index = pLinks++;
		if (!pAddresses.valid())
			pAddresses = gen::Module->table(u8"linked_addresses", true);
		return entry;
	}

	/* allocate the address to this module */
	entry.thisModule = true;
	entry.incomplete = true;
	entry.function = gen::Module->function(str::Format<str::u8::Local<128>>(u8"addr_{:#018x}", address), pBlockPrototype, wasm::Export{});
	pQueue.push({ address, depth });
	return entry;
}

gen::detail::PlaceAddress gen::detail::Addresses::pushLocal(env::guest_t address) {
	Placement& entry = fPush(address, pDepth);
	return detail::PlaceAddress{ entry.function, entry.index, entry.thisModule, entry.alreadyExists };
}
void gen::detail::Addresses::pushRoot(env::guest_t address) {
	fPush(address, 0);
}

void gen::detail::Addresses::setup(const wasm::Prototype& blockPrototype) {
	pBlockPrototype = blockPrototype;
}
const wasm::Table& gen::detail::Addresses::addresses() {
	return pAddresses;
}
bool gen::detail::Addresses::empty() const {
	return pQueue.empty();
}
gen::detail::OpenAddress gen::detail::Addresses::start() {
	Queued next = pQueue.top();
	pQueue.pop();

	/* mark the block as complete and start it */
	Placement& entry = pTranslated[next.address];
	entry.incomplete = false;
	pDepth = next.depth + 1;
	return { entry.function, next.address };
}
std::vector<env::BlockExport> gen::detail::Addresses::close(const detail::MappingState& mappingState) {
	/* setup the addresses-table limit */
	if (pAddresses.valid())
		gen::Module->limit(pAddresses, wasm::Limit{ uint32_t(pLinks), uint32_t(pLinks) });

	/* ensure that all functions have been written and collect the exports */
	std::vector<env::BlockExport> exports;
	for (const auto& [address, place] : pTranslated) {
		if (!place.thisModule)
			continue;
		std::u8string name{ place.function.id() };
		if (place.incomplete)
			logger.fatal(u8"Block Address [", name, u8"] has not been produced");
		exports.push_back({ name, address });
	}

	/* setup the startup-function for the already existing imports */
	if (pNeedsStartup) {
		wasm::Function startup = gen::Module->function(u8"_setup_imports", {}, {});
		wasm::Sink sink{ startup };

		/* bind the sink (necessary for the writer) */
		gen::Instance()->setSink(&sink);
		detail::MappingWriter mapping{ mappingState };

		/* iterate over the functions and write them all to the imports */
		for (const auto& [address, place] : pTranslated) {
			if (place.thisModule || !place.alreadyExists)
				continue;

			/* perform the lookup (will only throw an exception, if the blocks were to be flushed mid block-creation) */
			sink[I::U32::Const(place.index)];
			sink[I::U64::Const(address)];
			mapping.makeGetFunction();
			sink[I::Table::Set(pAddresses)];
		}

		/* clear the sink reference and bind the startup-function */
		gen::Instance()->setSink(0);
		gen::Module->startup(startup);
	}
	return exports;
}
