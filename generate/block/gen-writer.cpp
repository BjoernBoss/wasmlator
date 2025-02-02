#include "../generate.h"

gen::FulFill::FulFill(const gen::Writer* writer, env::guest_t address, uint32_t cacheIndex, gen::MemoryType type, Operation operation) : pWriter{ writer }, pAddress{ address }, pCacheIndex{ cacheIndex }, pType{ type }, pOperation{ operation } {}
void gen::FulFill::now() {
	/* dont reset after the operation, as mutliple fulfills might happend due to case-switches */
	if (pOperation == Operation::memory)
		pWriter->pMemory.makeEndWrite(pCacheIndex, pType, pAddress);
	else if (pOperation == Operation::context)
		pWriter->pContext.makeEndWrite(pType);
	else if (pOperation == Operation::host)
		pWriter->pContext.makeEndHostWrite(pType);
}

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
gen::FulFill gen::Writer::write(uint32_t cacheIndex, gen::MemoryType type, env::guest_t instAddress) const {
	pMemory.makeStartWrite(cacheIndex, type, instAddress);
	return gen::FulFill{ this, instAddress, cacheIndex, type, gen::FulFill::Operation::memory };
}
void gen::Writer::get(uint32_t offset, gen::MemoryType type) const {
	pContext.makeRead(offset, type);
}
gen::FulFill gen::Writer::set(uint32_t offset, gen::MemoryType type) const {
	pContext.makeStartWrite(offset, type);
	return gen::FulFill{ this, 0, 0, type, gen::FulFill::Operation::context };
}
void gen::Writer::readHost(const void* host, gen::MemoryType type) const {
	pContext.makeHostRead(host, type);
}
gen::FulFill gen::Writer::writeHost(void* host, gen::MemoryType type) const {
	pContext.makeStartHostWrite(host);
	return gen::FulFill{ this, 0, 0, type, gen::FulFill::Operation::host };
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
