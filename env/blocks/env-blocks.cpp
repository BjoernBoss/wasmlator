#include "../env-process.h"

namespace I = wasm::inst;

env::Blocks::Blocks(env::Process* process) : pProcess{ process } {}

uint32_t env::Blocks::fLookup(env::guest_t address) const {
	pProcess->debug(str::Format<std::u8string>(u8"Lookup block: [{:#018x}]", address));
	auto it = pMapping.find(address);
	if (it == pMapping.end())
		return 0;
	return it->second;
}
void env::Blocks::fAssociate(env::guest_t address, uint32_t index) {
	pProcess->debug(str::Format<std::u8string>(u8"Assigning [{:#018x}] to [{}]", address, index));
	pMapping.insert({ address, index });
}
void env::Blocks::fFlushed() {
	pProcess->debug(u8"Flushed blocks");
	pMapping.clear();
}

void env::Blocks::makeGoto(const wasm::Variable& i64Address, const env::ModuleState& state) const {
	wasm::Sink& sink = i64Address.sink();
	sink[I::Local::Get(i64Address)];
	sink[I::Call::Tail(state.blocks.execute)];
}
void env::Blocks::makeLookup(const wasm::Variable& i64Address, const env::ModuleState& state) const {
	wasm::Sink& sink = i64Address.sink();
	sink[I::Local::Get(i64Address)];
	sink[I::Call::Direct(state.blocks.lookup)];
}

void env::Blocks::execute(env::guest_t address) {
	bridge::Blocks::Execute(pProcess->context().id(), address);
}
void env::Blocks::flushBlocks() {
	bridge::Blocks::FlushBlocks(pProcess->context().id());
}
