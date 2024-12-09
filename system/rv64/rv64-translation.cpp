#include "rv64-translation.h"

namespace I = wasm::inst;

wasm::Variable rv64::Translate::fTemp32(bool first) {
	wasm::Variable& var = (first ? pTemp32_0 : pTemp32_1);
	if (!var.valid())
		var = pWriter->sink().local(wasm::Type::i32, (first ? u8"_temp_i32_0" : u8"_temp_i32_1"));
	return var;
}
wasm::Variable rv64::Translate::fTemp64(bool first) {
	wasm::Variable& var = (first ? pTemp64_0 : pTemp64_1);
	if (!var.valid())
		var = pWriter->sink().local(wasm::Type::i64, (first ? u8"_temp_i64_0" : u8"_temp_i64_1"));
	return var;
}

bool rv64::Translate::fLoadSrc1(bool forceNull, bool half) const {
	if (pInst->src1 != reg::Zero) {
		pWriter->get(pInst->src1 * sizeof(uint64_t), half ? gen::MemoryType::i32 : gen::MemoryType::i64);
		return true;
	}
	else if (forceNull)
		pWriter->sink()[half ? I::U32::Const(0) : I::U64::Const(0)];
	return false;
}
bool rv64::Translate::fLoadSrc2(bool forceNull, bool half) const {
	if (pInst->src2 != reg::Zero) {
		pWriter->get(pInst->src2 * sizeof(uint64_t), half ? gen::MemoryType::i32 : gen::MemoryType::i64);
		return true;
	}
	else if (forceNull)
		pWriter->sink()[half ? I::U32::Const(0) : I::U64::Const(0)];
	return false;
}
void rv64::Translate::fStoreDest() const {
	pWriter->set(pInst->dest * sizeof(uint64_t), gen::MemoryType::i64);
}

void rv64::Translate::fMakeJAL() {
	wasm::Sink& sink = pWriter->sink();

	/* write the next pc to the destination register */
	if (pInst->dest != reg::Zero) {
		sink[I::U64::Const(pNextAddress)];
		fStoreDest();
	}

	/* check if its an indirect jump and if it can be predicted somehow (based on the riscv specification) */
	if (pInst->opcode == rv64::Opcode::jump_and_link_reg) {
		wasm::Variable addr = fTemp64(true);

		/* write the target address to the stack */
		sink[I::I64::Const(pInst->imm)];
		if (fLoadSrc1(false, false))
			sink[I::I64::Add()];

		/* perform the alignment validation */
		sink[I::Local::Tee(addr)];
		sink[I::U64::Shrink()];
		sink[I::U32::Const(1)];
		sink[I::U32::And()];
		{
			wasm::IfThen _if{ sink };
			pContext->throwException(Translate::MisAlignedException, *pWriter, pAddress, pNextAddress);
		}
		sink[I::Local::Get(addr)];

		/* check if the instruction can be considered a return */
		if (pInst->isRet())
			pWriter->ret();

		/* check if the instruction can be considered a call */
		else if (pInst->isCall())
			pWriter->call(pNextAddress);

		/* translate the instruction as normal jump */
		else
			pWriter->jump();
		return;
	}

	/* check if the target is misaligned and add the misalignment-exception */
	env::guest_t address = (pAddress + pInst->imm);
	if (pInst->isMisAligned(pAddress))
		pContext->throwException(Translate::MisAlignedException, *pWriter, pAddress, pNextAddress);

	/* add the instruction as normal direct call or jump */
	else if (pInst->isCall())
		pWriter->call(address, pNextAddress);
	else
		pWriter->jump(address);
}
void rv64::Translate::fMakeBranch() const {
	env::guest_t address = pAddress + pInst->imm;
	wasm::Sink& sink = pWriter->sink();

	/* check if the branch can be discarded, as it is misaligned */
	if (pInst->isMisAligned(pAddress)) {
		pContext->throwException(Translate::MisAlignedException, *pWriter, pAddress, pNextAddress);
		return;
	}

	/* check if the condition can be discarded */
	if (pInst->src1 == pInst->src2) {
		if (pInst->opcode == rv64::Opcode::branch_eq || pInst->opcode == rv64::Opcode::branch_ge_s || pInst->opcode == rv64::Opcode::branch_ge_u)
			pWriter->jump(address);
		return;
	}

	/* write the result of the condition to the stack */
	if (pInst->opcode == rv64::Opcode::branch_eq) {
		if (!fLoadSrc1(false, false) || !fLoadSrc2(false, false))
			sink[I::U64::EqualZero()];
		else
			sink[I::U64::Equal()];
	}
	else {
		fLoadSrc1(true, false);
		fLoadSrc2(true, false);
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
	const wasm::Target* target = pWriter->hasTarget(address);
	if (target != 0) {
		sink[I::Branch::If(*target)];
		return;
	}

	/* add the optional jump to the target */
	wasm::IfThen _if{ sink };
	pWriter->jump(address);
}
void rv64::Translate::fMakeALUImm() const {
	wasm::Sink& sink = pWriter->sink();

	/* check if the operation can be discarded */
	if (pInst->dest == reg::Zero)
		return;

	/* write the result of the operation to the stack */
	switch (pInst->opcode) {
	case rv64::Opcode::add_imm:
		if (pInst->imm == 0)
			fLoadSrc1(true, false);
		else {
			sink[I::I64::Const(pInst->imm)];
			if (fLoadSrc1(false, false))
				sink[I::U64::Add()];
		}
		break;
	case rv64::Opcode::add_imm_half:
		if (pInst->imm == 0)
			fLoadSrc1(true, true);
		else {
			sink[I::I32::Const(pInst->imm)];
			if (fLoadSrc1(false, true))
				sink[I::U32::Add()];
		}
		sink[I::I32::Expand()];
		break;
	case rv64::Opcode::xor_imm:
		sink[I::I64::Const(pInst->imm)];
		if (fLoadSrc1(false, false))
			sink[I::U64::XOr()];
		break;
	case rv64::Opcode::or_imm:
		sink[I::I64::Const(pInst->imm)];
		if (fLoadSrc1(false, false))
			sink[I::U64::Or()];
		break;
	case rv64::Opcode::and_imm:
		if (fLoadSrc1(true, false)) {
			sink[I::I64::Const(pInst->imm)];
			sink[I::U64::And()];
		}
		break;
	case rv64::Opcode::shift_left_logic_imm:
		/* no need to mask shift as immediate is already restricted to 6 bits */
		if (fLoadSrc1(true, false)) {
			sink[I::I64::Const(pInst->imm)];
			sink[I::U64::ShiftLeft()];
		}
		break;
	case rv64::Opcode::shift_left_logic_imm_half:
		/* no need to mask shift as immediate is already restricted to 6 bits */
		if (fLoadSrc1(true, true)) {
			sink[I::I32::Const(pInst->imm)];
			sink[I::U32::ShiftLeft()];
		}
		sink[I::I32::Expand()];
		break;
	case rv64::Opcode::shift_right_logic_imm:
		/* no need to mask shift as immediate is already restricted to 6 bits */
		if (fLoadSrc1(true, false)) {
			sink[I::I64::Const(pInst->imm)];
			sink[I::U64::ShiftRight()];
		}
		break;
	case rv64::Opcode::shift_right_logic_imm_half:
		/* no need to mask shift as immediate is already restricted to 6 bits */
		if (fLoadSrc1(true, true)) {
			sink[I::I32::Const(pInst->imm)];
			sink[I::U32::ShiftRight()];
		}
		sink[I::I32::Expand()];
		break;
	case rv64::Opcode::shift_right_arith_imm:
		/* no need to mask shift as immediate is already restricted to 6 bits */
		if (fLoadSrc1(true, false)) {
			sink[I::I64::Const(pInst->imm)];
			sink[I::I64::ShiftRight()];
		}
		break;
	case rv64::Opcode::shift_right_arith_imm_half:
		/* no need to mask shift as immediate is already restricted to 6 bits */
		if (fLoadSrc1(true, true)) {
			sink[I::I32::Const(pInst->imm)];
			sink[I::I32::ShiftRight()];
		}
		sink[I::I32::Expand()];
		break;
	case rv64::Opcode::set_less_than_s_imm:
		if (fLoadSrc1(false, false)) {
			sink[I::I64::Const(pInst->imm)];
			sink[I::I64::Less()];
			sink[I::U32::Expand()];
		}
		else
			sink[I::U64::Const(pInst->imm > 0 ? 1 : 0)];
		break;
	case rv64::Opcode::set_less_than_u_imm:
		if (fLoadSrc1(false, false)) {
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
	wasm::Sink& sink = pWriter->sink();

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
		fLoadSrc1(false, false);
		if (pInst->src1 != reg::Zero && fLoadSrc2(false, false))
			sink[I::U64::Add()];
		break;
	case rv64::Opcode::add_reg_half:
		fLoadSrc1(false, true);
		if (pInst->src1 != reg::Zero && fLoadSrc2(false, true))
			sink[I::U32::Add()];
		sink[I::I32::Expand()];
		break;
	case rv64::Opcode::sub_reg:
		if (fLoadSrc1(true, false) && fLoadSrc2(false, false))
			sink[I::U64::Sub()];
		break;
	case rv64::Opcode::sub_reg_half:
		if (fLoadSrc1(true, true) && fLoadSrc2(false, true))
			sink[I::U32::Sub()];
		sink[I::I32::Expand()];
		break;
	case rv64::Opcode::xor_reg:
		fLoadSrc1(false, false);
		if (pInst->src1 != reg::Zero && fLoadSrc2(false, false))
			sink[I::U64::XOr()];
		break;
	case rv64::Opcode::or_reg:
		fLoadSrc1(false, false);
		if (pInst->src1 != reg::Zero && fLoadSrc2(false, false))
			sink[I::U64::Or()];
		break;
	case rv64::Opcode::and_reg:
		if (pInst->src1 == reg::Zero || pInst->src2 == reg::Zero)
			sink[I::U64::Const(0)];
		else {
			fLoadSrc1(false, false);
			fLoadSrc2(false, false);
			sink[I::U64::And()];
		}
		break;
	case rv64::Opcode::shift_left_logic_reg:
		/* wasm-standard automatically performs masking of value, identical to requirements of riscv */
		if (fLoadSrc1(true, false) && fLoadSrc2(false, false))
			sink[I::U64::ShiftLeft()];
		break;
	case rv64::Opcode::shift_left_logic_reg_half:
		/* wasm-standard automatically performs masking of value, identical to requirements of riscv */
		if (fLoadSrc1(true, true) && fLoadSrc2(false, true))
			sink[I::U32::ShiftLeft()];
		sink[I::I32::Expand()];
		break;
	case rv64::Opcode::shift_right_logic_reg:
		/* wasm-standard automatically performs masking of value, identical to requirements of riscv */
		if (fLoadSrc1(true, false) && fLoadSrc2(false, false))
			sink[I::U64::ShiftRight()];
		break;
	case rv64::Opcode::shift_right_logic_reg_half:
		/* wasm-standard automatically performs masking of value, identical to requirements of riscv */
		if (fLoadSrc1(true, true) && fLoadSrc2(false, true))
			sink[I::U32::ShiftRight()];
		sink[I::I32::Expand()];
		break;
	case rv64::Opcode::shift_right_arith_reg:
		/* wasm-standard automatically performs masking of value, identical to requirements of riscv */
		if (fLoadSrc1(true, false) && fLoadSrc2(false, false))
			sink[I::I64::ShiftRight()];
		break;
	case rv64::Opcode::shift_right_arith_reg_half:
		/* wasm-standard automatically performs masking of value, identical to requirements of riscv */
		if (fLoadSrc1(true, true) && fLoadSrc2(false, true))
			sink[I::I32::ShiftRight()];
		sink[I::I32::Expand()];
		break;
	case rv64::Opcode::set_less_than_s_reg:
		fLoadSrc1(true, false);
		fLoadSrc2(true, false);
		sink[I::I64::Less()];
		sink[I::U32::Expand()];
		break;
	case rv64::Opcode::set_less_than_u_reg:
		fLoadSrc1(true, false);
		fLoadSrc2(true, false);
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
	wasm::Sink& sink = pWriter->sink();

	/* check if the operation can be discarded */
	if (pInst->dest == reg::Zero)
		return;

	/* compute the destination address and write it to the stack */
	sink[I::I64::Const(pInst->imm)];
	if (fLoadSrc1(false, false))
		sink[I::U64::Add()];

	/* perform the actual load of the value (memory-register maps 1-to-1 to memory-cache no matter if read or write) */
	switch (pInst->opcode) {
	case rv64::Opcode::load_byte_s:
		pWriter->read(pInst->src1, gen::MemoryType::i8To64, pAddress);
		break;
	case rv64::Opcode::load_half_s:
		pWriter->read(pInst->src1, gen::MemoryType::i16To64, pAddress);
		break;
	case rv64::Opcode::load_word_s:
		pWriter->read(pInst->src1, gen::MemoryType::i32To64, pAddress);
		break;
	case rv64::Opcode::load_byte_u:
		pWriter->read(pInst->src1, gen::MemoryType::u8To64, pAddress);
		break;
	case rv64::Opcode::load_half_u:
		pWriter->read(pInst->src1, gen::MemoryType::u16To64, pAddress);
		break;
	case rv64::Opcode::load_word_u:
		pWriter->read(pInst->src1, gen::MemoryType::u32To64, pAddress);
		break;
	case rv64::Opcode::load_dword:
		pWriter->read(pInst->src1, gen::MemoryType::i64, pAddress);
		break;
	default:
		host::Fatal(u8"Unexpected opcode [", size_t(pInst->opcode), u8"] encountered");
		break;
	}

	/* write the value to the register */
	fStoreDest();
}
void rv64::Translate::fMakeStore() const {
	wasm::Sink& sink = pWriter->sink();

	/* compute the destination address and write it to the stack */
	sink[I::I64::Const(pInst->imm)];
	if (fLoadSrc1(false, false))
		sink[I::U64::Add()];

	/* write the source value to the stack */
	fLoadSrc2(true, false);

	/* perform the actual store of the value (memory-register maps 1-to-1 to memory-cache no matter if read or write) */
	switch (pInst->opcode) {
	case rv64::Opcode::store_byte:
		pWriter->write(pInst->src1, gen::MemoryType::u8To64, pAddress);
		break;
	case rv64::Opcode::store_half:
		pWriter->write(pInst->src1, gen::MemoryType::u16To64, pAddress);
		break;
	case rv64::Opcode::store_word:
		pWriter->write(pInst->src1, gen::MemoryType::u32To64, pAddress);
		break;
	case rv64::Opcode::store_dword:
		pWriter->write(pInst->src1, gen::MemoryType::i64, pAddress);
		break;
	default:
		host::Fatal(u8"Unexpected opcode [", size_t(pInst->opcode), u8"] encountered");
		break;
	}
}
void rv64::Translate::fMakeMul() const {
	wasm::Sink& sink = pWriter->sink();

	/* check if the operation can be discarded */
	if (pInst->dest == reg::Zero)
		return;
	if (pInst->src1 == reg::Zero || pInst->src2 == reg::Zero) {
		sink[I::U64::Const(0)];
		fStoreDest();
		return;
	}

	/* write the result of the operation to the stack */
	switch (pInst->opcode) {
	case rv64::Opcode::mul_reg:
		fLoadSrc1(false, false);
		fLoadSrc2(false, false);
		sink[I::I64::Mul()];
		break;
	case rv64::Opcode::mul_reg_half:
		fLoadSrc1(false, true);
		fLoadSrc2(false, true);
		sink[I::U32::Mul()];
		sink[I::I32::Expand()];
		break;
	default:
		host::Fatal(u8"Unexpected opcode [", size_t(pInst->opcode), u8"] encountered");
		break;
	}

	/* write the result to the register */
	fStoreDest();
}
void rv64::Translate::fMakeDivRem() {
	wasm::Sink& sink = pWriter->sink();

	/* operation-checks to simplify the logic */
	bool half = (pInst->opcode == rv64::Opcode::div_s_reg_half || pInst->opcode == rv64::Opcode::div_u_reg_half ||
		pInst->opcode == rv64::Opcode::rem_s_reg_half || pInst->opcode == rv64::Opcode::rem_u_reg_half);
	bool div = (pInst->opcode == rv64::Opcode::div_s_reg_half || pInst->opcode == rv64::Opcode::div_u_reg_half ||
		pInst->opcode == rv64::Opcode::div_s_reg || pInst->opcode == rv64::Opcode::div_u_reg);
	bool sign = (pInst->opcode == rv64::Opcode::div_s_reg || pInst->opcode == rv64::Opcode::div_s_reg_half ||
		pInst->opcode == rv64::Opcode::rem_s_reg || pInst->opcode == rv64::Opcode::rem_s_reg_half);
	wasm::Type type = (half ? wasm::Type::i32 : wasm::Type::i64);

	/* check if the operation can be discarded, as it is a division by zero */
	if (pInst->dest == reg::Zero)
		return;
	if (pInst->src2 == reg::Zero) {
		if (div) {
			sink[I::I64::Const(-1)];
			fStoreDest();
		}
		else if (pInst->src1 != pInst->dest) {
			fLoadSrc1(true, false);
			fStoreDest();
		}
		return;
	}

	/* allocate the temporary variables and write the operands to the stack */
	wasm::Variable temp = (half ? fTemp32(true) : fTemp64(true));
	fLoadSrc1(true, half);
	fLoadSrc2(true, half);

	/* check if a division-by-zero will occur */
	sink[I::Local::Tee(temp)];
	sink[half ? I::U32::EqualZero() : I::U64::EqualZero()];
	wasm::IfThen zero{ sink, u8"", { type }, {} };

	/* write the division-by-zero result to the stack (for remainder, its just the first operand) */
	if (div) {
		sink[I::Drop()];
		sink[I::I64::Const(-1)];
	}
	fStoreDest();

	/* restore the second operand */
	zero.otherwise();
	sink[I::Local::Get(temp)];

	/* check if a division overflow may occur */
	wasm::Block overflown;
	if (sign) {
		overflown = std::move(wasm::Block{ sink, u8"", { type, type }, {} });
		sink[half ? I::I32::Const(-1) : I::I64::Const(-1)];
		sink[half ? I::U32::Equal() : I::U64::Equal()];
		wasm::IfThen mayOverflow{ sink, u8"", { type }, { type, type } };

		/* check the second operand (temp can be overwritten, as the divisor is now known) */
		sink[I::Local::Tee(temp)];
		sink[half ? I::I32::Const(std::numeric_limits<int32_t>::min()) : I::I64::Const(std::numeric_limits<int64_t>::min())];
		sink[half ? I::U32::Equal() : I::U64::Equal()];
		{
			wasm::IfThen isOverflow{ sink, u8"", {}, { type, type } };

			/* write the overflow result to the destination */
			if (div)
				sink[half ? I::I32::Const(std::numeric_limits<int32_t>::min()) : I::I64::Const(std::numeric_limits<int64_t>::min())];
			else
				sink[half ? I::I32::Const(0) : I::I64::Const(0)];
			fStoreDest();
			sink[I::Branch::Direct(overflown)];

			/* restore the operands for the operation */
			isOverflow.otherwise();
			sink[I::Local::Get(temp)];
			sink[half ? I::I32::Const(-1) : I::I64::Const(-1)];
		}

		/* restore the first operand */
		mayOverflow.otherwise();
		sink[I::Local::Get(temp)];
	}

	/* write the result of the operation to the stack */
	switch (pInst->opcode) {
	case rv64::Opcode::div_s_reg:
		sink[I::I64::Div()];
		break;
	case rv64::Opcode::div_s_reg_half:
		sink[I::I32::Div()];
		sink[I::I32::Expand()];
		break;
	case rv64::Opcode::div_u_reg:
		sink[I::U64::Div()];
		break;
	case rv64::Opcode::div_u_reg_half:
		sink[I::U32::Div()];
		sink[I::I32::Expand()];
		break;
	case rv64::Opcode::rem_s_reg:
		sink[I::I64::Mod()];
		break;
	case rv64::Opcode::rem_s_reg_half:
		sink[I::I32::Mod()];
		sink[I::I32::Expand()];
		break;
	case rv64::Opcode::rem_u_reg:
		sink[I::U64::Mod()];
		break;
	case rv64::Opcode::rem_u_reg_half:
		sink[I::U32::Mod()];
		sink[I::I32::Expand()];
		break;
	default:
		host::Fatal(u8"Unexpected opcode [", size_t(pInst->opcode), u8"] encountered");
		break;
	}

	/* write the result to the register */
	fStoreDest();
}
void rv64::Translate::fMakeAMO(bool half) {
	/*
	*	Note: simply treat the operations as all being atomic, as no threading is supported
	*/
	wasm::Sink& sink = pWriter->sink();

	/* load the source address into the temporary */
	wasm::Variable addr = fTemp64(true);
	fLoadSrc1(true, false);
	sink[I::Local::Tee(addr)];

	/* validate the alignment constraints of the address */
	sink[I::U64::Shrink()];
	sink[I::U32::Const(half ? 0x03 : 0x07)];
	sink[I::U32::And()];
	{
		wasm::IfThen _if{ sink };
		pContext->throwException(Translate::MisAlignedException, *pWriter, pAddress, pNextAddress);
	}

	/* perform the reading of the original value and write it immediately to the destination */
	wasm::Variable value = (half ? fTemp32(true) : fTemp64(false));
	sink[I::Local::Get(addr)];
	pWriter->read(pInst->src1, (half ? gen::MemoryType::i32 : gen::MemoryType::i64), pAddress);
	sink[I::Local::Tee(value)];
	if (half)
		sink[I::I32::Expand()];
	fStoreDest();

	/* write the destination address to the stack */
	sink[I::Local::Get(addr)];

	/* write the new value to the stack (perform the operation in the corresponding width) */
	switch (pInst->opcode) {
	case rv64::Opcode::amo_swap_w:
	case rv64::Opcode::amo_swap_d:
		fLoadSrc2(true, half);
		break;
	case rv64::Opcode::amo_add_w:
	case rv64::Opcode::amo_add_d:
		sink[I::Local::Get(value)];
		if (fLoadSrc2(false, half))
			sink[half ? I::U32::Add() : I::U64::Add()];
		break;
	case rv64::Opcode::amo_xor_w:
	case rv64::Opcode::amo_xor_d:
		sink[I::Local::Get(value)];
		if (fLoadSrc2(false, half))
			sink[half ? I::U32::XOr() : I::U64::XOr()];
		break;
	case rv64::Opcode::amo_and_w:
	case rv64::Opcode::amo_and_d:
		if (fLoadSrc2(true, half)) {
			sink[I::Local::Get(value)];
			sink[half ? I::U32::And() : I::U64::And()];
		}
		break;
	case rv64::Opcode::amo_or_w:
	case rv64::Opcode::amo_or_d:
		sink[I::Local::Get(value)];
		if (fLoadSrc2(false, half))
			sink[half ? I::U32::Or() : I::U64::Or()];
		break;
	case rv64::Opcode::amo_min_s_w:
	case rv64::Opcode::amo_min_s_d:
		if (fLoadSrc2(true, half)) {
			wasm::Variable other = (half ? fTemp32(false) : addr);
			sink[I::Local::Tee(other)];
			sink[I::Local::Get(value)];
			sink[I::Local::Get(other)];
		}
		else {
			sink[I::Local::Get(value)];
			sink[half ? I::U32::Const(0) : I::U64::Const(0)];
		}
		sink[I::Local::Get(value)];
		sink[half ? I::I32::Less() : I::I64::Less()];
		sink[I::Select()];
		break;
	case rv64::Opcode::amo_max_s_w:
	case rv64::Opcode::amo_max_s_d:
		if (fLoadSrc2(true, half)) {
			wasm::Variable other = (half ? fTemp32(false) : addr);
			sink[I::Local::Tee(other)];
			sink[I::Local::Get(value)];
			sink[I::Local::Get(other)];
		}
		else {
			sink[I::Local::Get(value)];
			sink[half ? I::U32::Const(0) : I::U64::Const(0)];
		}
		sink[I::Local::Get(value)];
		sink[half ? I::I32::Greater() : I::I64::Greater()];
		sink[I::Select()];
		break;
	case rv64::Opcode::amo_min_u_w:
	case rv64::Opcode::amo_min_u_d:
		if (fLoadSrc2(true, half)) {
			wasm::Variable other = (half ? fTemp32(false) : addr);
			sink[I::Local::Tee(other)];
			sink[I::Local::Get(value)];
			sink[I::Local::Get(other)];
			sink[I::Local::Get(value)];
			sink[half ? I::U32::Less() : I::U64::Less()];
			sink[I::Select()];
		}
		break;
	case rv64::Opcode::amo_max_u_w:
	case rv64::Opcode::amo_max_u_d:
		if (fLoadSrc2(false, half)) {
			wasm::Variable other = (half ? fTemp32(false) : addr);
			sink[I::Local::Tee(other)];
			sink[I::Local::Get(value)];
			sink[I::Local::Get(other)];
			sink[I::Local::Get(value)];
			sink[half ? I::U32::Greater() : I::U64::Greater()];
			sink[I::Select()];
		}
		else
			sink[I::Local::Get(value)];
		break;
	default:
		host::Fatal(u8"Unexpected opcode [", size_t(pInst->opcode), u8"] encountered");
		break;
	}

	/* write the result back to the memory */
	pWriter->write(pInst->src1, (half ? gen::MemoryType::i32 : gen::MemoryType::i64), pAddress);
}
void rv64::Translate::fMakeAMOLR() {
	/*
	*	Note: simply treat the operations as all being atomic, as no threading is supported
	*/
	wasm::Sink& sink = pWriter->sink();

	/* operation-checks to simplify the logic */
	bool half = (pInst->opcode == rv64::Opcode::load_reserved_w);

	/* load the source address into the temporary */
	wasm::Variable addr = fTemp64(true);
	fLoadSrc1(true, false);
	sink[I::Local::Tee(addr)];

	/* validate the alignment constraints of the address */
	sink[I::U64::Shrink()];
	sink[I::U32::Const(half ? 0x03 : 0x07)];
	sink[I::U32::And()];
	{
		wasm::IfThen _if{ sink };
		pContext->throwException(Translate::MisAlignedException, *pWriter, pAddress, pNextAddress);
	}

	/* write the value from memory to the register */
	sink[I::Local::Get(addr)];
	pWriter->read(pInst->src1, (half ? gen::MemoryType::i32To64 : gen::MemoryType::i64), pAddress);
	fStoreDest();
}
void rv64::Translate::fMakeAMOSC() {
	/*
	*	Note: simply treat the operations as all being atomic, as no threading is supported
	*/
	wasm::Sink& sink = pWriter->sink();

	/* operation-checks to simplify the logic */
	bool half = (pInst->opcode == rv64::Opcode::load_reserved_w);

	/* load the source address into the temporary */
	wasm::Variable addr = fTemp64(true);
	fLoadSrc1(true, false);
	sink[I::Local::Tee(addr)];

	/* validate the alignment constraints of the address */
	sink[I::U64::Shrink()];
	sink[I::U32::Const(half ? 0x03 : 0x07)];
	sink[I::U32::And()];
	{
		wasm::IfThen _if{ sink };
		pContext->throwException(Translate::MisAlignedException, *pWriter, pAddress, pNextAddress);
	}

	/* write the source value to the address (assumption that reservation is always valid) */
	sink[I::Local::Get(addr)];
	fLoadSrc2(true, half);
	pWriter->write(pInst->src1, (half ? gen::MemoryType::i32 : gen::MemoryType::i64), pAddress);

	/* write the result to the destination register */
	sink[I::U64::Const(0)];
	fStoreDest();
}
void rv64::Translate::fMakeCSR() const {

}

void rv64::Translate::resetAll(sys::ExecContext* context, const gen::Writer* writer) {
	pTemp32_0 = wasm::Variable{};
	pTemp32_1 = wasm::Variable{};
	pTemp64_0 = wasm::Variable{};
	pTemp64_1 = wasm::Variable{};
	pContext = context;
	pWriter = writer;
}
void rv64::Translate::start(env::guest_t address) {
	pAddress = address;
	pNextAddress = address;
}
void rv64::Translate::next(const rv64::Instruction& inst) {
	/* setup the state for the upcoming instruction */
	pAddress = pNextAddress;
	pNextAddress += inst.size;
	pInst = &inst;

	/* perform the actual translation */
	wasm::Sink& sink = pWriter->sink();
	switch (pInst->opcode) {
	case rv64::Opcode::misaligned:
		pContext->throwException(Translate::MisAlignedException, *pWriter, pAddress, pNextAddress);
		break;
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
	case rv64::Opcode::ecall:
		pContext->syscall(*pWriter, pAddress, pNextAddress);
		break;
	case rv64::Opcode::ebreak:
		pContext->throwException(Translate::EBreakException, *pWriter, pAddress, pNextAddress);
		break;
	case rv64::Opcode::fence:
		pContext->flushMemCache(*pWriter, pAddress, pNextAddress);
		break;
	case rv64::Opcode::fence_inst:
		pContext->flushInstCache(*pWriter, pAddress, pNextAddress);
		break;
	case rv64::Opcode::mul_reg:
	case rv64::Opcode::mul_reg_half:
		fMakeMul();
		break;
	case rv64::Opcode::div_s_reg:
	case rv64::Opcode::div_s_reg_half:
	case rv64::Opcode::div_u_reg:
	case rv64::Opcode::div_u_reg_half:
	case rv64::Opcode::rem_s_reg:
	case rv64::Opcode::rem_s_reg_half:
	case rv64::Opcode::rem_u_reg:
	case rv64::Opcode::rem_u_reg_half:
		fMakeDivRem();
		break;
	case rv64::Opcode::load_reserved_w:
	case rv64::Opcode::load_reserved_d:
		fMakeAMOLR();
		break;
	case rv64::Opcode::store_conditional_w:
	case rv64::Opcode::store_conditional_d:
		fMakeAMOSC();
		break;
	case rv64::Opcode::amo_swap_w:
	case rv64::Opcode::amo_add_w:
	case rv64::Opcode::amo_xor_w:
	case rv64::Opcode::amo_and_w:
	case rv64::Opcode::amo_or_w:
	case rv64::Opcode::amo_min_s_w:
	case rv64::Opcode::amo_max_s_w:
	case rv64::Opcode::amo_min_u_w:
	case rv64::Opcode::amo_max_u_w:
		fMakeAMO(true);
		break;
	case rv64::Opcode::amo_swap_d:
	case rv64::Opcode::amo_add_d:
	case rv64::Opcode::amo_xor_d:
	case rv64::Opcode::amo_and_d:
	case rv64::Opcode::amo_or_d:
	case rv64::Opcode::amo_min_s_d:
	case rv64::Opcode::amo_max_s_d:
	case rv64::Opcode::amo_min_u_d:
	case rv64::Opcode::amo_max_u_d:
		fMakeAMO(false);
		break;
	case rv64::Opcode::csr_read_write:
	case rv64::Opcode::csr_read_and_set:
	case rv64::Opcode::csr_read_and_clear:
	case rv64::Opcode::csr_read_write_imm:
	case rv64::Opcode::csr_read_and_set_imm:
	case rv64::Opcode::csr_read_and_clear_imm:
		fMakeCSR();
		break;


	case rv64::Opcode::mul_high_s_reg:
	case rv64::Opcode::mul_high_s_u_reg:
	case rv64::Opcode::mul_high_u_reg:
	case rv64::Opcode::_invalid:
		host::Fatal(u8"Instruction [", size_t(pInst->opcode), u8"] currently not implemented");
	}
}
