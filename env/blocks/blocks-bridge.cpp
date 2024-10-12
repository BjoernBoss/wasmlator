#include "../env-process.h"
#include "../../interface/interface.h"

uint32_t env::bridge::Blocks::LookupComplex(uint64_t process, uint64_t address) {
	return reinterpret_cast<env::Process*>(process)->blocks().fLookup(address);
}
void env::bridge::Blocks::Flushed(uint64_t process) {
	return reinterpret_cast<env::Process*>(process)->blocks().fFlushed();
}
void env::bridge::Blocks::Associate(uint64_t process, uint64_t address, uint32_t index) {
	return reinterpret_cast<env::Process*>(process)->blocks().fAssociate(address, index);
}

void env::bridge::Blocks::Execute(env::id_t id, env::guest_t address) {
	blocks_execute(id, address);
}
void env::bridge::Blocks::FlushBlocks(env::id_t id) {
	blocks_flush_blocks(id);
}
