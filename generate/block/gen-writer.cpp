#include "gen-writer.h"

gen::Writer::Writer(wasm::Sink& sink, detail::SuperBlock& block, const detail::MemoryState& memory, const detail::ContextState& context, const detail::MappingState& mapping, detail::Addresses& addresses, const detail::InteractState& interact) : pSink{ sink }, pSuperBlock{ block }, pMemory{ memory, sink }, pContext{ context, sink }, pAddress{ mapping, addresses, sink }, pInteract{ interact, sink } {}

wasm::Sink& gen::Writer::sink() const {
	return pSink;
}
const wasm::Target* gen::Writer::hasTarget(const gen::Instruction& inst) const {
	return pSuperBlock.lookup(inst);
}
void gen::Writer::read(uint32_t cacheIndex, gen::MemoryType type, const gen::Instruction& inst) const {
	pMemory.makeRead(cacheIndex, type, inst.address);
}
void gen::Writer::code(uint32_t cacheIndex, gen::MemoryType type, const gen::Instruction& inst) const {
	pMemory.makeCode(cacheIndex, type, inst.address);
}
void gen::Writer::write(uint32_t cacheIndex, gen::MemoryType type, const gen::Instruction& inst) const {
	pMemory.makeWrite(cacheIndex, type, inst.address);
}
void gen::Writer::ctxRead(uint32_t offset, gen::MemoryType type) const {
	pContext.makeRead(offset, type);
}
void gen::Writer::ctxWrite(uint32_t offset, gen::MemoryType type) const {
	pContext.makeWrite(offset, type);
}
void gen::Writer::terminate(const gen::Instruction& inst) const {
	pContext.makeTerminate(inst.address);
}
void gen::Writer::call(env::guest_t address, const gen::Instruction& inst) const {
	pAddress.makeCall(address, inst.address + inst.size);
}
void gen::Writer::call(const gen::Instruction& inst) const {
	pAddress.makeCallIndirect(inst.address + inst.size);
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
