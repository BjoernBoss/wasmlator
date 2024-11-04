#include "../env-process.h"
#include "../../interface/interface.h"

uint32_t env::bridge::Mapping::Resolve(uint64_t process, uint64_t address) {
	return reinterpret_cast<env::Process*>(process)->mapping().fResolve(address);
}
void env::bridge::Mapping::Flushed(uint64_t process) {
	return reinterpret_cast<env::Process*>(process)->mapping().fFlushed();
}
void env::bridge::Mapping::Associate(uint64_t process, uint64_t address, uint32_t index) {
	return reinterpret_cast<env::Process*>(process)->mapping().fAssociate(address, index);
}

uint32_t env::bridge::Mapping::Execute(env::id_t id, env::guest_t address) {
	return map_execute(id, address);
}
void env::bridge::Mapping::Flush(env::id_t id) {
	map_flush_blocks(id);
}
