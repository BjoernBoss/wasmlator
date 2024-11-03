#include "../env-process.h"

env::Mapping::Mapping(env::Process* process) : pProcess{ process } {}

uint32_t env::Mapping::fResolve(env::guest_t address) const {
	pProcess->debug(str::Format<std::u8string>(u8"Lookup block: [{:#018x}]", address));
	auto it = pMapping.find(address);
	if (it == pMapping.end())
		return 0;
	return it->second;
}
void env::Mapping::fAssociate(env::guest_t address, uint32_t index) {
	pProcess->debug(str::Format<std::u8string>(u8"Assigning [{:#018x}] to [{}]", address, index));
	pMapping.insert({ address, index });
}
void env::Mapping::fFlushed() {
	pProcess->debug(u8"Flushed blocks");
	pMapping.clear();
}

void env::Mapping::execute(env::guest_t address) {
	bridge::Mapping::Execute(pProcess->id(), address);
}
bool env::Mapping::contains(env::guest_t address) const {
	return (pMapping.find(address) != pMapping.end());
}
void env::Mapping::flush() {
	bridge::Mapping::Flush(pProcess->id());
}
