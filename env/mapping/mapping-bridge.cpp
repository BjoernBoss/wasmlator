#include "../env-process.h"
#include "../../interface/interface.h"

uint32_t env::detail::MappingBridge::Resolve(uint64_t address) {
	return env::Instance()->mapping().fResolve(address);
}
void env::detail::MappingBridge::Flushed() {
	env::Instance()->mapping().fFlushed();
}
void env::detail::MappingBridge::Associate(uint64_t address, uint32_t index) {
	env::Instance()->mapping().fAssociate(address, index);
}

uint32_t env::detail::MappingBridge::Execute(env::guest_t address) {
	return map_execute(address);
}
void env::detail::MappingBridge::Flush() {
	map_flush_blocks();
}
