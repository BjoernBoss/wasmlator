#include "gen-writer.h"

namespace I = wasm::inst;

gen::Writer::Writer(wasm::Sink& sink, detail::SuperBlock& block, const detail::MemoryState& memory, const detail::ContextState& context, const detail::MappingState& mapping, detail::Addresses& addresses, const detail::InteractState& interact) :
	pSink{ sink }, pSuperBlock{ block }, pMemory{ memory, sink }, pContext{ context, sink }, pAddress{ mapping, addresses, sink }, pInteract{ interact, sink } {}

wasm::Sink& gen::Writer::sink() const {
	return pSink;
}
const wasm::Target* gen::Writer::hasTarget(env::guest_t target) const {
	detail::InstTarget lookup = pSuperBlock.lookup(target);
	return (lookup.directJump ? lookup.target : 0);
}
void gen::Writer::jump(env::guest_t target) const {
	/* check if a branch can be inserted */
	detail::InstTarget lookup = pSuperBlock.lookup(target);
	if (lookup.target != 0)
		pSink[I::Branch::Direct(*lookup.target)];

	/* add the redirecting jump */
	else
		pAddress.makeJump(target);
}
void gen::Writer::jump() const {
	pAddress.makeJumpIndirect();
}
void gen::Writer::call(env::guest_t target, env::guest_t nextAddress) const {
	pAddress.makeCall(target, nextAddress);
}
void gen::Writer::call(env::guest_t nextAddress) const {
	pAddress.makeCallIndirect(nextAddress);
}
void gen::Writer::ret() const {
	pAddress.makeReturn();
}
void gen::Writer::read(uint32_t cacheIndex, gen::MemoryType type, env::guest_t instAddress) const {
	pMemory.makeRead(cacheIndex, type, instAddress);
}
void gen::Writer::code(uint32_t cacheIndex, gen::MemoryType type, env::guest_t instAddress) const {
	pMemory.makeCode(cacheIndex, type, instAddress);
}
void gen::Writer::write(uint32_t cacheIndex, gen::MemoryType type, env::guest_t instAddress) const {
	pMemory.makeWrite(cacheIndex, type, instAddress);
}
void gen::Writer::get(uint32_t offset, gen::MemoryType type) const {
	pContext.makeRead(offset, type);
}
void gen::Writer::set(uint32_t offset, gen::MemoryType type) const {
	pContext.makeWrite(offset, type);
}
void gen::Writer::terminate(env::guest_t instAddress) const {
	pContext.makeTerminate(instAddress);
}
void gen::Writer::invokeVoid(uint32_t index) const {
	pInteract.makeVoid(index);
}
void gen::Writer::invokeParam(uint32_t index) const {
	pInteract.makeParam(index);
}
