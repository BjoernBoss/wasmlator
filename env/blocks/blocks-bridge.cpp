#include "blocks-bridge.h"
#include "../env-process.h"
#include "../../interface/interface.h"

uint32_t env::bridge::Blocks::LookupComplex(uint64_t process, uint64_t address) {
	return reinterpret_cast<env::Process*>(process)->blocks().fLookup(address);
}

void env::bridge::Blocks::FlushBlocks(env::id_t id) {
	blocks_flush_blocks(id);
}
