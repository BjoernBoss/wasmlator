#include "rv64-translation.h"

namespace I = wasm::inst;

rv64::Translate::Translate(const gen::Writer& writer, env::guest_t address, uint32_t ebreak) : pWriter{ writer }, pAddress{ address }, pNextAddress{ address }, pEBreakId{ ebreak } {}

bool rv64::Translate::fLoadSrc1(bool forceNull) const {
	if (pInst->src1 != reg::Zero) {
		pWriter.get(pInst->src1 * sizeof(uint64_t), gen::MemoryType::i64);
		return true;
	}
	else if (forceNull)
		pWriter.sink()[I::U64::Const(0)];
	return false;
}
bool rv64::Translate::fLoadSrc1Half(bool forceNull) const {
	if (pInst->src1 != reg::Zero) {
		pWriter.get(pInst->src1 * sizeof(uint64_t), gen::MemoryType::i32);
		return true;
	}
	else if (forceNull)
		pWriter.sink()[I::U32::Const(0)];
	return false;
}
bool rv64::Translate::fLoadSrc2(bool forceNull) const {
	if (pInst->src2 != reg::Zero) {
		pWriter.get(pInst->src2 * sizeof(uint64_t), gen::MemoryType::i64);
		return true;
	}
	else if (forceNull)
		pWriter.sink()[I::U64::Const(0)];
	return false;
}
bool rv64::Translate::fLoadSrc2Half(bool forceNull) const {
	if (pInst->src2 != reg::Zero) {
		pWriter.get(pInst->src2 * sizeof(uint64_t), gen::MemoryType::i32);
		return true;
	}
	else if (forceNull)
		pWriter.sink()[I::U32::Const(0)];
	return false;
}
void rv64::Translate::fStoreDest() const {
	pWriter.set(pInst->dest * sizeof(uint64_t), gen::MemoryType::i64);
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
		if (fLoadSrc1(false))
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
	else
		pWriter.jump(address);
}
void rv64::Translate::fMakeBranch() const {
	env::guest_t address = pAddress + pInst->imm;
	wasm::Sink& sink = pWriter.sink();

	/* check if the condition can be discarded */
	if (pInst->src1 == pInst->src2) {
		if (pInst->opcode == rv64::Opcode::branch_eq || pInst->opcode == rv64::Opcode::branch_ge_s || pInst->opcode == rv64::Opcode::branch_ge_u)
			pWriter.jump(address);
		return;
	}

	/* write the result of the condition to the stack */
	if (pInst->opcode == rv64::Opcode::branch_eq) {
		if (!fLoadSrc1(false) || !fLoadSrc2(false))
			sink[I::U64::EqualZero()];
		else
			sink[I::U64::Equal()];
	}
	else {
		fLoadSrc1(true);
		fLoadSrc2(true);
		switch (pInst->opcode) {
		case rv64::Opcode::branch_ne:
			sink[I::U64::NotEqual()];
			break;
		case rv64::Opcode::branch_lt_s:
			sink[I::I64::Less()];
			break;
		case rv64::Opcode::branch_ge_s:
			sink[I::I64::GreaterEqual()];
			break;
		case rv64::Opcode::branch_lt_u:
			sink[I::U64::Less()];
			break;
		case rv64::Opcode::branch_ge_u:
			sink[I::U64::GreaterEqual()];
			break;
		default:
			host::Fatal(u8"Unexpected opcode [", size_t(pInst->opcode), u8"] encountered");
			break;
		}
	}

	/* check if the target can directly be branched to */
	const wasm::Target* target = pWriter.hasTarget(address);
	if (target != 0) {
		sink[I::Branch::If(*target)];
		return;
	}

	/* add the optional jump to the target */
	wasm::IfThen _if{ sink };
	pWriter.jump(address);
}
void rv64::Translate::fMakeALUImm() const {
	wasm::Sink& sink = pWriter.sink();

	/* check if the operation can be discarded */
	if (pInst->dest == reg::Zero)
		return;

	/* write the result of the operation to the stack */
	switch (pInst->opcode) {
	case rv64::Opcode::add_imm:
		sink[I::I64::Const(pInst->imm)];
		if (fLoadSrc1(false))
			sink[I::U64::Add()];
		break;
	case rv64::Opcode::add_imm_half:
		sink[I::I32::Const(pInst->imm)];
		if (fLoadSrc1Half(false))
			sink[I::U32::Add()];
		sink[I::I32::Expand()];
		break;
	case rv64::Opcode::xor_imm:
		sink[I::I64::Const(pInst->imm)];
		if (fLoadSrc1(false))
			sink[I::U64::XOr()];
		break;
	case rv64::Opcode::or_imm:
		sink[I::I64::Const(pInst->imm)];
		if (fLoadSrc1(false))
			sink[I::U64::Or()];
		break;
	case rv64::Opcode::and_imm:
		if (fLoadSrc1(true)) {
			sink[I::I64::Const(pInst->imm)];
			sink[I::U64::And()];
		}
		break;
	case rv64::Opcode::shift_left_logic_imm:
		/* no need to mask shift as immediate is already restricted to 6 bits */
		if (fLoadSrc1(true)) {
			sink[I::I64::Const(pInst->imm)];
			sink[I::U64::ShiftLeft()];
		}
		break;
	case rv64::Opcode::shift_left_logic_imm_half:
		/* no need to mask shift as immediate is already restricted to 6 bits */
		if (fLoadSrc1Half(true)) {
			sink[I::I32::Const(pInst->imm)];
			sink[I::U32::ShiftLeft()];
		}
		sink[I::I32::Expand()];
		break;
	case rv64::Opcode::shift_right_logic_imm:
		/* no need to mask shift as immediate is already restricted to 6 bits */
		if (fLoadSrc1(true)) {
			sink[I::I64::Const(pInst->imm)];
			sink[I::U64::ShiftRight()];
		}
		break;
	case rv64::Opcode::shift_right_logic_imm_half:
		/* no need to mask shift as immediate is already restricted to 6 bits */
		if (fLoadSrc1Half(true)) {
			sink[I::I32::Const(pInst->imm)];
			sink[I::U32::ShiftRight()];
		}
		sink[I::I32::Expand()];
		break;
	case rv64::Opcode::shift_right_arith_imm:
		/* no need to mask shift as immediate is already restricted to 6 bits */
		if (fLoadSrc1(true)) {
			sink[I::I64::Const(pInst->imm)];
			sink[I::I64::ShiftRight()];
		}
		break;
	case rv64::Opcode::shift_right_arith_imm_half:
		/* no need to mask shift as immediate is already restricted to 6 bits */
		if (fLoadSrc1Half(true)) {
			sink[I::I32::Const(pInst->imm)];
			sink[I::I32::ShiftRight()];
		}
		sink[I::I32::Expand()];
		break;
	case rv64::Opcode::set_less_than_s_imm:
		if (fLoadSrc1(false)) {
			sink[I::I64::Const(pInst->imm)];
			sink[I::I64::Less()];
			sink[I::U32::Expand()];
		}
		else
			sink[I::U64::Const(pInst->imm > 0 ? 1 : 0)];
		break;
	case rv64::Opcode::set_less_than_u_imm:
		if (fLoadSrc1(false)) {
			sink[I::I64::Const(pInst->imm)];
			sink[I::U64::Less()];
			sink[I::U32::Expand()];
		}
		else
			sink[I::U64::Const(pInst->imm != 0 ? 1 : 0)];
		break;
	default:
		host::Fatal(u8"Unexpected opcode [", size_t(pInst->opcode), u8"] encountered");
		break;
	}

	/* write the result to the register */
	fStoreDest();
}
void rv64::Translate::fMakeALUReg() const {
	wasm::Sink& sink = pWriter.sink();

	/* check if the operation can be discarded */
	if (pInst->dest == reg::Zero)
		return;

	/* check if the two sources are null, in which case nothing needs to be done */
	if (pInst->src1 == reg::Zero && pInst->src2 == reg::Zero) {
		sink[I::U64::Const(0)];
		fStoreDest();
		return;
	}

	/* write the result of the operation to the stack */
	switch (pInst->opcode) {
	case rv64::Opcode::add_reg:
		if (fLoadSrc1(false) && fLoadSrc2(false))
			sink[I::U64::Add()];
		break;
	case rv64::Opcode::add_reg_half:
		if (fLoadSrc1Half(false) && fLoadSrc2Half(false))
			sink[I::U32::Add()];
		sink[I::I32::Expand()];
		break;
	case rv64::Opcode::sub_reg:
		if (fLoadSrc1(true) && fLoadSrc2(false))
			sink[I::U64::Sub()];
		break;
	case rv64::Opcode::sub_reg_half:
		if (fLoadSrc1Half(false) && fLoadSrc2Half(false))
			sink[I::U32::Sub()];
		sink[I::I32::Expand()];
		break;
	case rv64::Opcode::xor_reg:
		if (fLoadSrc1(false) && fLoadSrc2(false))
			sink[I::U64::XOr()];
		break;
	case rv64::Opcode::or_reg:
		if (fLoadSrc1(false) && fLoadSrc2(false))
			sink[I::U64::Or()];
		break;
	case rv64::Opcode::and_reg:
		if (pInst->src1 == reg::Zero || pInst->src2 == reg::Zero)
			sink[I::U64::Const(0)];
		else {
			fLoadSrc1(false);
			fLoadSrc2(false);
			sink[I::U64::And()];
		}
		break;
	case rv64::Opcode::shift_left_logic_reg:
		/* wasm-standard automatically performs masking of value, identical to requirements of riscv */
		if (fLoadSrc1(true) && fLoadSrc2(false))
			sink[I::U64::ShiftLeft()];
		break;
	case rv64::Opcode::shift_left_logic_reg_half:
		/* wasm-standard automatically performs masking of value, identical to requirements of riscv */
		if (fLoadSrc1Half(true) && fLoadSrc2Half(false))
			sink[I::U32::ShiftLeft()];
		sink[I::I32::Expand()];
		break;
	case rv64::Opcode::shift_right_logic_reg:
		/* wasm-standard automatically performs masking of value, identical to requirements of riscv */
		if (fLoadSrc1(true) && fLoadSrc2(false))
			sink[I::U64::ShiftRight()];
		break;
	case rv64::Opcode::shift_right_logic_reg_half:
		/* wasm-standard automatically performs masking of value, identical to requirements of riscv */
		if (fLoadSrc1Half(true) && fLoadSrc2Half(false))
			sink[I::U32::ShiftRight()];
		sink[I::I32::Expand()];
		break;
	case rv64::Opcode::shift_right_arith_reg:
		/* wasm-standard automatically performs masking of value, identical to requirements of riscv */
		if (fLoadSrc1(true) && fLoadSrc2(false))
			sink[I::I64::ShiftRight()];
		break;
	case rv64::Opcode::shift_right_arith_reg_half:
		/* wasm-standard automatically performs masking of value, identical to requirements of riscv */
		if (fLoadSrc1Half(true) && fLoadSrc2Half(false))
			sink[I::I32::ShiftRight()];
		sink[I::I32::Expand()];
		break;
	case rv64::Opcode::set_less_than_s_reg:
		fLoadSrc1(true);
		fLoadSrc2(true);
		sink[I::I64::Less()];
		sink[I::U32::Expand()];
		break;
	case rv64::Opcode::set_less_than_u_reg:
		fLoadSrc1(true);
		fLoadSrc2(true);
		sink[I::U64::Less()];
		sink[I::U32::Expand()];
		break;
	default:
		host::Fatal(u8"Unexpected opcode [", size_t(pInst->opcode), u8"] encountered");
		break;
	}

	/* write the result to the register */
	fStoreDest();
}
void rv64::Translate::fMakeLoad() const {
	wasm::Sink& sink = pWriter.sink();

	/* check if the operation can be discarded */
	if (pInst->dest == reg::Zero)
		return;

	/* compute the destination address and write it to the stack */
	sink[I::I64::Const(pInst->imm)];
	if (fLoadSrc1(false))
		sink[I::U64::Add()];

	/* perform the actual load of the value (memory-register maps 1-to-1 to memory-cache no matter if read or write) */
	switch (pInst->opcode) {
	case rv64::Opcode::load_byte_s:
		pWriter.read(pInst->src1, gen::MemoryType::i8To64, pAddress);
		break;
	case rv64::Opcode::load_half_s:
		pWriter.read(pInst->src1, gen::MemoryType::i16To64, pAddress);
		break;
	case rv64::Opcode::load_word_s:
		pWriter.read(pInst->src1, gen::MemoryType::i32To64, pAddress);
		break;
	case rv64::Opcode::load_byte_u:
		pWriter.read(pInst->src1, gen::MemoryType::u8To64, pAddress);
		break;
	case rv64::Opcode::load_half_u:
		pWriter.read(pInst->src1, gen::MemoryType::u16To64, pAddress);
		break;
	case rv64::Opcode::load_word_u:
		pWriter.read(pInst->src1, gen::MemoryType::u32To64, pAddress);
		break;
	case rv64::Opcode::load_dword:
		pWriter.read(pInst->src1, gen::MemoryType::i64, pAddress);
		break;
	default:
		host::Fatal(u8"Unexpected opcode [", size_t(pInst->opcode), u8"] encountered");
		break;
	}

	/* write the value to the register */
	fStoreDest();
}
void rv64::Translate::fMakeStore() const {
	wasm::Sink& sink = pWriter.sink();

	/* compute the destination address and write it to the stack */
	sink[I::I64::Const(pInst->imm)];
	if (fLoadSrc1(false))
		sink[I::U64::Add()];

	/* write the source value to the stack */
	fLoadSrc2(true);

	/* perform the actual store of the value (memory-register maps 1-to-1 to memory-cache no matter if read or write) */
	switch (pInst->opcode) {
	case rv64::Opcode::store_byte:
		pWriter.write(pInst->src1, gen::MemoryType::u8To64, pAddress);
		break;
	case rv64::Opcode::store_half:
		pWriter.read(pInst->src1, gen::MemoryType::u16To64, pAddress);
		break;
	case rv64::Opcode::store_word:
		pWriter.read(pInst->src1, gen::MemoryType::u32To64, pAddress);
		break;
	case rv64::Opcode::store_dword:
		pWriter.read(pInst->src1, gen::MemoryType::i64, pAddress);
		break;
	default:
		host::Fatal(u8"Unexpected opcode [", size_t(pInst->opcode), u8"] encountered");
		break;
	}
}

void rv64::Translate::next(const rv64::Instruction& inst) {
	/* setup the state for the upcoming instruction */
	pAddress = pNextAddress;
	pNextAddress += (inst.compressed ? 2 : 4);
	pInst = &inst;

	/* perform the actual translation */
	wasm::Sink& sink = pWriter.sink();
	switch (pInst->opcode) {
	case rv64::Opcode::load_upper_imm:
		if (pInst->dest != reg::Zero) {
			sink[I::U64::Const(pInst->imm)];
			fStoreDest();
		}
		break;
	case rv64::Opcode::add_upper_imm_pc:
		if (pInst->dest != reg::Zero) {
			sink[I::U64::Const(pAddress + pInst->imm)];
			fStoreDest();
		}
		break;
	case rv64::Opcode::jump_and_link_imm:
	case rv64::Opcode::jump_and_link_reg:
		fMakeJAL();
		break;
	case rv64::Opcode::branch_eq:
	case rv64::Opcode::branch_ne:
	case rv64::Opcode::branch_lt_s:
	case rv64::Opcode::branch_ge_s:
	case rv64::Opcode::branch_lt_u:
	case rv64::Opcode::branch_ge_u:
		fMakeBranch();
		break;
	case rv64::Opcode::add_imm:
	case rv64::Opcode::xor_imm:
	case rv64::Opcode::or_imm:
	case rv64::Opcode::and_imm:
	case rv64::Opcode::shift_left_logic_imm:
	case rv64::Opcode::shift_right_logic_imm:
	case rv64::Opcode::shift_right_arith_imm:
	case rv64::Opcode::set_less_than_s_imm:
	case rv64::Opcode::set_less_than_u_imm:
	case rv64::Opcode::add_imm_half:
	case rv64::Opcode::shift_left_logic_imm_half:
	case rv64::Opcode::shift_right_logic_imm_half:
	case rv64::Opcode::shift_right_arith_imm_half:
		fMakeALUImm();
		break;
	case rv64::Opcode::add_reg:
	case rv64::Opcode::sub_reg:
	case rv64::Opcode::xor_reg:
	case rv64::Opcode::or_reg:
	case rv64::Opcode::and_reg:
	case rv64::Opcode::set_less_than_s_reg:
	case rv64::Opcode::set_less_than_u_reg:
	case rv64::Opcode::shift_left_logic_reg:
	case rv64::Opcode::shift_right_logic_reg:
	case rv64::Opcode::shift_right_arith_reg:
	case rv64::Opcode::add_reg_half:
	case rv64::Opcode::sub_reg_half:
	case rv64::Opcode::shift_left_logic_reg_half:
	case rv64::Opcode::shift_right_logic_reg_half:
	case rv64::Opcode::shift_right_arith_reg_half:
		fMakeALUReg();
		break;
	case rv64::Opcode::load_byte_s:
	case rv64::Opcode::load_half_s:
	case rv64::Opcode::load_word_s:
	case rv64::Opcode::load_byte_u:
	case rv64::Opcode::load_half_u:
	case rv64::Opcode::load_word_u:
	case rv64::Opcode::load_dword:
		fMakeLoad();
		break;
	case rv64::Opcode::store_byte:
	case rv64::Opcode::store_half:
	case rv64::Opcode::store_word:
	case rv64::Opcode::store_dword:
		fMakeStore();
		break;
	case rv64::Opcode::ebreak:
		pWriter.invokeVoid(pEBreakId);
		break;

	case rv64::Opcode::fence:
	case rv64::Opcode::fence_inst:
	case rv64::Opcode::ecall:
	case rv64::Opcode::csr_read_write:
	case rv64::Opcode::csr_read_and_set:
	case rv64::Opcode::csr_read_and_clear:
	case rv64::Opcode::csr_read_write_imm:
	case rv64::Opcode::csr_read_and_set_imm:
	case rv64::Opcode::csr_read_and_clear_imm:
	case rv64::Opcode::mul_reg:
	case rv64::Opcode::mul_reg_half:
	case rv64::Opcode::mul_high_s_reg:
	case rv64::Opcode::mul_high_s_u_reg:
	case rv64::Opcode::mul_high_u_reg:
	case rv64::Opcode::div_s_reg:
	case rv64::Opcode::div_s_reg_half:
	case rv64::Opcode::div_u_reg:
	case rv64::Opcode::div_u_reg_half:
	case rv64::Opcode::rem_s_reg:
	case rv64::Opcode::rem_s_reg_half:
	case rv64::Opcode::rem_u_reg:
	case rv64::Opcode::rem_u_reg_half:
	case rv64::Opcode::_invalid:
		host::Fatal(u8"Instruction [", size_t(pInst->opcode), u8"] currently not implemented");
	}
}
