#include "gen-writer.h"

gen::Writer::Writer(wasm::Sink& sink, detail::SuperBlock& block, const detail::MemoryState& memory, const detail::ContextState& context, const detail::MappingState& mapping, detail::Addresses& addresses, const detail::InteractState& interact) : pSink{ sink }, pSuperBlock{ block }, pMemory{ memory, sink }, pContext{ context, sink }, pAddress{ mapping, addresses, sink }, pInteract{ interact, sink } {}

wasm::Sink& gen::Writer::sink() const {
	return pSink;
}
const wasm::Target* gen::Writer::hasTarget(env::guest_t address) const {
	return pSuperBlock.lookup(address);
}
void gen::Writer::read(uint32_t cacheIndex, gen::MemoryType type) const {
	pMemory.makeRead(cacheIndex, type);
}
void gen::Writer::code(uint32_t cacheIndex, gen::MemoryType type) const {
	pMemory.makeCode(cacheIndex, type);
}
void gen::Writer::write(const wasm::Variable& value, uint32_t cacheIndex, gen::MemoryType type) const {
	pMemory.makeWrite(value, cacheIndex, type);
}
void gen::Writer::terminate() const {
	pContext.makeTerminate();
}
void gen::Writer::ctxRead(uint32_t offset, gen::MemoryType type) const {
	pContext.makeRead(offset, type);
}
void gen::Writer::ctxWrite(const wasm::Variable& value, uint32_t offset, gen::MemoryType type) const {
	pContext.makeWrite(value, offset, type);
}
void gen::Writer::call(env::guest_t address, env::guest_t nextAddress) const {
	pAddress.makeCall(address, nextAddress);
}
void gen::Writer::call(env::guest_t nextAddress) const {
	pAddress.makeCallIndirect(nextAddress);
}
void gen::Writer::jump(env::guest_t address) const {
	pAddress.makeJump(address);
}
void gen::Writer::jump() const {
	pAddress.makeJumpIndirect();
}
void gen::Writer::ret() const {
	pAddress.makeReturn();
}
void gen::Writer::invokeVoid(uint32_t index) const {
	pInteract.makeVoid(index);
}
void gen::Writer::invokeParam(uint32_t index) const {
	pInteract.makeParam(index);
}
