#include "../env-process.h"

uint32_t env::Mapping::fResolve(env::guest_t address) const {
	host::Debug(str::Format<std::u8string>(u8"Lookup block: [{:#018x}]", address));
	auto it = pMapping.find(address);
	if (it == pMapping.end())
		return 0;
	return it->second;
}
void env::Mapping::fAssociate(env::guest_t address, uint32_t index) {
	host::Debug(str::Format<std::u8string>(u8"Assigning [{:#018x}] to [{}]", address, index));
	pMapping.insert({ address, index });
}
void env::Mapping::fFlushed() {
	host::Debug(u8"Flushed blocks");
	pMapping.clear();
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
