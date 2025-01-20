#include "../generate.h"

gen::Writer::Writer(detail::SuperBlock& block, const detail::MemoryState& memory, const detail::ContextState& context, const detail::MappingState& mapping, detail::Addresses& addresses, const detail::InteractState& interact) :
	pSuperBlock{ block }, pMemory{ memory }, pContext{ context }, pAddress{ mapping, addresses }, pInteract{ interact } {
}

const wasm::Target* gen::Writer::hasTarget(env::guest_t target) const {
	detail::InstTarget lookup = pSuperBlock.lookup(target);
	return (lookup.conditional ? 0 : lookup.target);
}
void gen::Writer::jump(env::guest_t target) const {
	/* check if a redirecting jump needs to be added */
	detail::InstTarget lookup = pSuperBlock.lookup(target);
	if (lookup.target == 0) {
		pAddress.makeJump(target);
		return;
	}

	/* write the index for the conditional branch out */
	if (lookup.conditional)
		gen::Add[I::U32::Const(lookup.index)];

	/* add the direct branch to the target */
	gen::Add[I::Branch::Direct(*lookup.target)];
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
void gen::Writer::write(uint32_t cacheIndex, gen::MemoryType type, env::guest_t instAddress) const {
	pMemory.makeWrite(cacheIndex, type, instAddress);
}
void gen::Writer::get(uint32_t offset, gen::MemoryType type) const {
	pContext.makeRead(offset, type);
}
void gen::Writer::set(uint32_t offset, gen::MemoryType type) const {
	pContext.makeWrite(offset, type);
}
void gen::Writer::readHost(const void* host, gen::MemoryType type) const {
	pContext.makeHostRead(host, type);
}
void gen::Writer::writeHost(void* host, gen::MemoryType type) const {
	pContext.makeHostWrite(host, type);
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
