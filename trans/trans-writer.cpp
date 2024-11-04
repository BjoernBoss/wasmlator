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
void trans::Writer::exit() const {
	pContext.makeExit();
}
void trans::Writer::ctxRead(uint32_t offset, trans::MemoryType type) const {
	pContext.makeRead(offset, type);
}
void trans::Writer::ctxWrite(const wasm::Variable& value, uint32_t offset, trans::MemoryType type) const {
	pContext.makeWrite(value, offset, type);
}
void trans::Writer::call(env::guest_t address, env::guest_t nextAddress) const {
	pAddress.makeCall(address, nextAddress);
}
void trans::Writer::call(env::guest_t nextAddress) const {
	pAddress.makeCallIndirect(nextAddress);
}
void trans::Writer::jump(env::guest_t address) const {
	pAddress.makeJump(address);
}
void trans::Writer::jump() const {
	pAddress.makeJumpIndirect();
}
void trans::Writer::ret() const {
	pAddress.makeReturn();
}
