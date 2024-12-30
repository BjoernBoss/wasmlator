#include "../generate.h"

gen::Writer::Writer(detail::SuperBlock& block, const detail::MemoryState& memory, const detail::ContextState& context, const detail::MappingState& mapping, detail::Addresses& addresses, const detail::InteractState& interact) :
	pSuperBlock{ block }, pMemory{ memory }, pContext{ context }, pAddress{ mapping, addresses }, pInteract{ interact } {
}

const wasm::Target* gen::Writer::hasTarget(env::guest_t target) const {
	detail::InstTarget lookup = pSuperBlock.lookup(target);
	return (lookup.directJump ? lookup.target : 0);
}
void gen::Writer::jump(env::guest_t target) const {
	/* check if a branch can be inserted */
	detail::InstTarget lookup = pSuperBlock.lookup(target);
	if (lookup.target != 0)
		gen::Add[I::Branch::Direct(*lookup.target)];

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
