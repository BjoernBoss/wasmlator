#include "rv64-translation.h"

namespace I = wasm::inst;

rv64::Translate::Translate(const gen::Writer& writer, env::guest_t address) : pWriter{ writer }, pAddress{ address - 4 }, pNextAddress{ address } {}

bool rv64::Translate::fLoadSrc1() const {
	if (pInst->src1 == reg::Zero)
		return false;
	pWriter.ctxRead(pInst->src1 * sizeof(uint64_t), gen::MemoryType::i64);
	return true;
}
bool rv64::Translate::fLoadSrc2() const {
	if (pInst->src2 == reg::Zero)
		return false;
	pWriter.ctxRead(pInst->src2 * sizeof(uint64_t), gen::MemoryType::i64);
	return true;
}
void rv64::Translate::fStoreDest() const {
	pWriter.ctxWrite(pInst->dest * sizeof(uint64_t), gen::MemoryType::i64);
}

void rv64::Translate::fMakeJAL() const {
	wasm::Sink& sink = pWriter.sink();

	/* write the next pc to the destination register */
	if (pInst->dest != reg::Zero) {
		sink[I::U64::Const(pNextAddress)];
		fStoreDest();
	}

	/* check if its an indirect jump and if it can be predicted somehow (based on the riscv specification) */
	if (pInst->opcode == rv64::Opcode::jump_and_link_reg) {
		/* write the target pc to the stack */
		sink[I::I64::Const(pInst->imm)];
		if (fLoadSrc1())
			sink[I::I64::Add()];

		/* check if the instruction can be considered a return */
		if (pInst->isRet())
			pWriter.ret();

		/* check if the instruction can be considered a call */
		else if (pInst->isCall())
			pWriter.call(pNextAddress);

		/* translate the instruction as normal jump */
		else
			pWriter.jump();
		return;
	}

	/* add the instruction as normal direct call or jump */
	env::guest_t address = (pAddress + pInst->imm);
	if (pInst->isCall())
		pWriter.call(address, pNextAddress);
	else if (const wasm::Target* target = pWriter.hasTarget(address); target != 0)
		sink[I::Branch::Direct(*target)];
	else
		pWriter.jump(address);
}

void rv64::Translate::next(const rv64::Instruction& inst) {
	/* setup the state for the upcoming instruction */
	pAddress = pNextAddress;
	pNextAddress += 4;
	pInst = &inst;

	/* perform the actual translation */
	wasm::Sink& sink = pWriter.sink();
	switch (inst.opcode) {
	case rv64::Opcode::load_upper_imm:
		if (inst.dest != reg::Zero) {
			sink[I::U64::Const(inst.imm)];
			fStoreDest();
		}
		break;
	case rv64::Opcode::add_upper_imm_pc:
		if (inst.dest != reg::Zero) {
			sink[I::U64::Const(pAddress + inst.imm)];
			fStoreDest();
		}
		break;
	case rv64::Opcode::add_imm:
		if (inst.dest != reg::Zero) {
			sink[I::I64::Const(inst.imm)];
			if (fLoadSrc1())
				sink[I::I64::Add()];
			fStoreDest();
		}
		break;
	case rv64::Opcode::jump_and_link_imm:
	case rv64::Opcode::jump_and_link_reg:
		fMakeJAL();
		break;
	default:
		host::Fatal(u8"Instruction [", size_t(inst.opcode), u8"] currently not implemented");
	}
}
