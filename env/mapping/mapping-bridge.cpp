#include "../env-process.h"
#include "../../interface/interface.h"

uint32_t env::detail::MappingBridge::Resolve(uint64_t address) {
	return env::Instance()->mapping().fResolve(address);
}
void env::detail::MappingBridge::Flushed() {
	env::Instance()->mapping().fFlushed();
}
void env::detail::MappingBridge::BlockLoaded(bool succeeded) {
	env::Instance()->mapping().fBlockLoaded(succeeded);
}

bool env::detail::MappingBridge::LoadBlock(const uint8_t* data, size_t size, size_t exports) {
	return (map_load_block(data, uint32_t(size), uint32_t(exports)) > 0);
}
uint32_t env::detail::MappingBridge::Define(const char8_t* name, size_t size, env::guest_t address) {
	return map_define(name, uint32_t(size), address);
}
void env::detail::MappingBridge::Flush() {
	map_flush_blocks();
}
uint32_t env::detail::MappingBridge::Execute(env::guest_t address) {
	return map_execute(address);
}