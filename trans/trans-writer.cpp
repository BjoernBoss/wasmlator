#include "trans-writer.h"

void trans::Writer::read(uint32_t cacheIndex, trans::MemoryType type) const {
	pMemory.makeRead(cacheIndex, type);
}
void trans::Writer::code(uint32_t cacheIndex, trans::MemoryType type) const {
	pMemory.makeCode(cacheIndex, type);
}
void trans::Writer::write(const wasm::Variable& value, uint32_t cacheIndex, trans::MemoryType type) const {
	pMemory.makeWrite(value, cacheIndex, type);
}
