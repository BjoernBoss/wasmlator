#include "rv64-translation.h"
#include "rv64-print.h"

static util::Logger logger{ u8"rv64::cpu" };

wasm::Variable rv64::Translate::fTemp32(size_t index) {
	wasm::Variable& var = pTemp[4 + index];
	if (!var.valid())
		var = gen::Sink->local(wasm::Type::i32, str::u8::Build(u8"_temp_i32_", index));
	return var;
}
wasm::Variable rv64::Translate::fTemp64(size_t index) {
	wasm::Variable& var = pTemp[index];
	if (!var.valid())
		var = gen::Sink->local(wasm::Type::i64, str::u8::Build(u8"_temp_i64_", index));
	return var;
}

bool rv64::Translate::fLoadSrc1(bool forceNull, bool half) const {
	if (pInst->src1 != reg::Zero) {
		gen::Make->get(offsetof(rv64::Context, iregs) + pInst->src1 * sizeof(uint64_t), half ? gen::MemoryType::i32 : gen::MemoryType::i64);
		return true;
	}
	else if (forceNull)
		gen::Add[half ? I::U32::Const(0) : I::U64::Const(0)];
	return false;
}
bool rv64::Translate::fLoadSrc2(bool forceNull, bool half) const {
	if (pInst->src2 != reg::Zero) {
		gen::Make->get(offsetof(rv64::Context, iregs) + pInst->src2 * sizeof(uint64_t), half ? gen::MemoryType::i32 : gen::MemoryType::i64);
		return true;
	}
	else if (forceNull)
		gen::Add[half ? I::U32::Const(0) : I::U64::Const(0)];
	return false;
}
void rv64::Translate::fStoreReg(uint8_t reg) const {
	gen::Make->set(offsetof(rv64::Context, iregs) + reg * sizeof(uint64_t), gen::MemoryType::i64);
}
void rv64::Translate::fStoreDest() const {
	fStoreReg(pInst->dest);
}

void rv64::Translate::fLoadFSrc1(bool half) const {
	gen::Make->get(offsetof(rv64::Context, fregs) + pInst->src1 * sizeof(double), half ? gen::MemoryType::f32 : gen::MemoryType::f64);
}
void rv64::Translate::fLoadFSrc2(bool half) const {
	gen::Make->get(offsetof(rv64::Context, fregs) + pInst->src2 * sizeof(double), half ? gen::MemoryType::f32 : gen::MemoryType::f64);
}
void rv64::Translate::fLoadFSrc3(bool half) const {
	gen::Make->get(offsetof(rv64::Context, fregs) + pInst->src3 * sizeof(double), half ? gen::MemoryType::f32 : gen::MemoryType::f64);
}
void rv64::Translate::fStoreFDest(bool isAsInt) const {
	if (isAsInt)
		gen::Make->get(offsetof(rv64::Context, fregs) + pInst->dest * sizeof(double), gen::MemoryType::i64);
	else
		gen::Make->get(offsetof(rv64::Context, fregs) + pInst->dest * sizeof(double), gen::MemoryType::f64);
}
void rv64::Translate::fExpandFloat(bool isAsInt, bool leaveAsInt) const {
	/* check if the value needs to be intereprted as an integer */
	if (!isAsInt)
		gen::Add[I::F32::AsInt()];

	/* perform the expansion to 64-bit doubles (upper 32bits are set to 1 - NaN Boxing) */
	gen::Add[I::U32::Expand()];
	gen::Add[I::U64::Const(0xffff'ffff'0000'0000)];
	gen::Add[I::U64::Or()];

	/* check if the value should be converted back to a float */
	if (!leaveAsInt)
		gen::Add[I::U64::AsFloat()];
}

void rv64::Translate::fMakeImms() const {
	/* check if the instruction can be skipped */
	if (pInst->dest == reg::Zero)
		return;

	/* write the immediate to the stack */
	switch (pInst->opcode) {
	case rv64::Opcode::multi_load_imm:
		gen::Add[I::U64::Const(pInst->imm)];
		break;
	case rv64::Opcode::load_upper_imm:
		gen::Add[I::U64::Const(pInst->imm)];
		break;
	case rv64::Opcode::add_upper_imm_pc:
	case rv64::Opcode::multi_load_address:
		gen::Add[I::U64::Const(pAddress + pInst->imm)];
		break;
	default:
		break;
	}

	/* write the immediate to the register */
	fStoreDest();
}
void rv64::Translate::fMakeJALI() {
	/* write the next pc to the destination register */
	if (pInst->opcode == rv64::Opcode::multi_call) {
		gen::Add[I::U64::Const(pNextAddress)];
		fStoreReg(reg::X1);
	}
	else if (pInst->opcode == rv64::Opcode::jump_and_link_imm && pInst->dest != reg::Zero) {
		gen::Add[I::U64::Const(pNextAddress)];
		fStoreDest();
	}

	/* check if the target is misaligned and add the misalignment-exception */
	env::guest_t address = (pAddress + pInst->imm);
	if (pInst->isMisAligned(pAddress))
		pWriter->makeException(Translate::MisAlignedException, pAddress, pNextAddress);

	/* add the instruction as normal direct call or jump */
	else if (pInst->isCall())
		gen::Make->call(address, pNextAddress);
	else
		gen::Make->jump(address);
}
void rv64::Translate::fMakeJALR() {
	/* write the next pc to the destination register */
	if (pInst->dest != reg::Zero) {
		gen::Add[I::U64::Const(pNextAddress)];
		fStoreDest();
	}
	wasm::Variable addr = fTemp64(0);

	/* write the target address to the stack */
	if (fLoadSrc1(false, false)) {
		if (pInst->imm != 0) {
			gen::Add[I::I64::Const(pInst->imm)];
			gen::Add[I::I64::Add()];
		}
	}
	else
		gen::Add[I::I64::Const(pInst->imm)];

	/* perform the alignment validation */
	gen::Add[I::Local::Tee(addr)];
	gen::Add[I::U64::Shrink()];
	gen::Add[I::U32::Const(1)];
	gen::Add[I::U32::And()];
	{
		wasm::IfThen _if{ gen::Sink };
		pWriter->makeException(Translate::MisAlignedException, pAddress, pNextAddress);
	}
	gen::Add[I::Local::Get(addr)];

	/* check if the instruction can be considered a return */
	if (pInst->isRet())
		gen::Make->ret();

	/* check if the instruction can be considered a call */
	else if (pInst->isCall())
		gen::Make->call(pNextAddress);

	/* translate the instruction as normal jump */
	else
		gen::Make->jump();
}
void rv64::Translate::fMakeBranch() const {
	env::guest_t address = pAddress + pInst->imm;

	/* check if the branch can be discarded, as it is misaligned */
	if (pInst->isMisAligned(pAddress)) {
		pWriter->makeException(Translate::MisAlignedException, pAddress, pNextAddress);
		return;
	}

	/* check if the condition can be discarded */
	if (pInst->src1 == pInst->src2) {
		if (pInst->opcode == rv64::Opcode::branch_eq || pInst->opcode == rv64::Opcode::branch_ge_s || pInst->opcode == rv64::Opcode::branch_ge_u)
			gen::Make->jump(address);
		return;
	}

	/* write the result of the condition to the stack */
	if (pInst->opcode == rv64::Opcode::branch_eq) {
		if (!fLoadSrc1(false, false) || !fLoadSrc2(false, false))
			gen::Add[I::U64::EqualZero()];
		else
			gen::Add[I::U64::Equal()];
	}
	else {
		fLoadSrc1(true, false);
		fLoadSrc2(true, false);
		switch (pInst->opcode) {
		case rv64::Opcode::branch_ne:
			gen::Add[I::U64::NotEqual()];
			break;
		case rv64::Opcode::branch_lt_s:
			gen::Add[I::I64::Less()];
			break;
		case rv64::Opcode::branch_ge_s:
			gen::Add[I::I64::GreaterEqual()];
			break;
		case rv64::Opcode::branch_lt_u:
			gen::Add[I::U64::Less()];
			break;
		case rv64::Opcode::branch_ge_u:
			gen::Add[I::U64::GreaterEqual()];
			break;
		default:
			break;
		}
	}

	/* check if the target can directly be branched to */
	const wasm::Target* target = gen::Make->hasTarget(address);
	if (target != 0) {
		gen::Add[I::Branch::If(*target)];
		return;
	}

	/* add the optional jump to the target */
	wasm::IfThen _if{ gen::Sink };
	gen::Make->jump(address);
}
void rv64::Translate::fMakeALUImm() const {
	/* check if the operation can be discarded */
	if (pInst->dest == reg::Zero)
		return;

	/* write the result of the operation to the stack */
	switch (pInst->opcode) {
	case rv64::Opcode::add_imm:
		if (pInst->imm == 0)
			fLoadSrc1(true, false);
		else {
			gen::Add[I::I64::Const(pInst->imm)];
			if (fLoadSrc1(false, false))
				gen::Add[I::U64::Add()];
		}
		break;
	case rv64::Opcode::add_imm_half:
		if (pInst->imm == 0)
			fLoadSrc1(true, true);
		else {
			gen::Add[I::I32::Const(pInst->imm)];
			if (fLoadSrc1(false, true))
				gen::Add[I::U32::Add()];
		}
		gen::Add[I::I32::Expand()];
		break;
	case rv64::Opcode::xor_imm:
		gen::Add[I::I64::Const(pInst->imm)];
		if (fLoadSrc1(false, false))
			gen::Add[I::U64::XOr()];
		break;
	case rv64::Opcode::or_imm:
		gen::Add[I::I64::Const(pInst->imm)];
		if (fLoadSrc1(false, false))
			gen::Add[I::U64::Or()];
		break;
	case rv64::Opcode::and_imm:
		if (fLoadSrc1(true, false)) {
			gen::Add[I::I64::Const(pInst->imm)];
			gen::Add[I::U64::And()];
		}
		break;
	case rv64::Opcode::shift_left_logic_imm:
		/* no need to mask shift as immediate is already restricted to 6 bits */
		if (fLoadSrc1(true, false)) {
			gen::Add[I::I64::Const(pInst->imm)];
			gen::Add[I::U64::ShiftLeft()];
		}
		break;
	case rv64::Opcode::shift_left_logic_imm_half:
		/* no need to mask shift as immediate is already restricted to 6 bits */
		if (fLoadSrc1(true, true)) {
			gen::Add[I::I32::Const(pInst->imm)];
			gen::Add[I::U32::ShiftLeft()];
		}
		gen::Add[I::I32::Expand()];
		break;
	case rv64::Opcode::shift_right_logic_imm:
		/* no need to mask shift as immediate is already restricted to 6 bits */
		if (fLoadSrc1(true, false)) {
			gen::Add[I::I64::Const(pInst->imm)];
			gen::Add[I::U64::ShiftRight()];
		}
		break;
	case rv64::Opcode::shift_right_logic_imm_half:
		/* no need to mask shift as immediate is already restricted to 6 bits */
		if (fLoadSrc1(true, true)) {
			gen::Add[I::I32::Const(pInst->imm)];
			gen::Add[I::U32::ShiftRight()];
		}
		gen::Add[I::I32::Expand()];
		break;
	case rv64::Opcode::shift_right_arith_imm:
		/* no need to mask shift as immediate is already restricted to 6 bits */
		if (fLoadSrc1(true, false)) {
			gen::Add[I::I64::Const(pInst->imm)];
			gen::Add[I::I64::ShiftRight()];
		}
		break;
	case rv64::Opcode::shift_right_arith_imm_half:
		/* no need to mask shift as immediate is already restricted to 6 bits */
		if (fLoadSrc1(true, true)) {
			gen::Add[I::I32::Const(pInst->imm)];
			gen::Add[I::I32::ShiftRight()];
		}
		gen::Add[I::I32::Expand()];
		break;
	case rv64::Opcode::set_less_than_s_imm:
		if (fLoadSrc1(false, false)) {
			gen::Add[I::I64::Const(pInst->imm)];
			gen::Add[I::I64::Less()];
			gen::Add[I::U32::Expand()];
		}
		else
			gen::Add[I::U64::Const(pInst->imm > 0 ? 1 : 0)];
		break;
	case rv64::Opcode::set_less_than_u_imm:
		if (fLoadSrc1(false, false)) {
			gen::Add[I::I64::Const(pInst->imm)];
			gen::Add[I::U64::Less()];
			gen::Add[I::U32::Expand()];
		}
		else
			gen::Add[I::U64::Const(pInst->imm != 0 ? 1 : 0)];
		break;
	default:
		break;
	}

	/* write the result to the register */
	fStoreDest();
}
void rv64::Translate::fMakeALUReg() const {
	/* check if the operation can be discarded */
	if (pInst->dest == reg::Zero)
		return;

	/* check if the two sources are null, in which case nothing needs to be done */
	if (pInst->src1 == reg::Zero && pInst->src2 == reg::Zero) {
		gen::Add[I::U64::Const(0)];
		fStoreDest();
		return;
	}

	/* check if the operation can be short-circuited */
	if ((pInst->src1 == reg::Zero || pInst->src2 == reg::Zero) && (pInst->opcode == rv64::Opcode::and_reg ||
		pInst->opcode == rv64::Opcode::mul_reg || pInst->opcode == rv64::Opcode::mul_reg_half)) {
		gen::Add[I::U64::Const(0)];
		fStoreDest();
		return;
	}

	/* write the result of the operation to the stack */
	switch (pInst->opcode) {
	case rv64::Opcode::add_reg:
		fLoadSrc1(false, false);
		fLoadSrc2(false, false);
		if (pInst->src1 != reg::Zero && pInst->src2 != reg::Zero)
			gen::Add[I::U64::Add()];
		break;
	case rv64::Opcode::add_reg_half:
		fLoadSrc1(false, true);
		fLoadSrc2(false, true);
		if (pInst->src1 != reg::Zero && pInst->src2 != reg::Zero)
			gen::Add[I::U32::Add()];
		gen::Add[I::I32::Expand()];
		break;
	case rv64::Opcode::sub_reg:
		fLoadSrc1(true, false);
		if (fLoadSrc2(false, false))
			gen::Add[I::U64::Sub()];
		break;
	case rv64::Opcode::sub_reg_half:
		fLoadSrc1(true, true);
		if (fLoadSrc2(false, true))
			gen::Add[I::U32::Sub()];
		gen::Add[I::I32::Expand()];
		break;
	case rv64::Opcode::xor_reg:
		fLoadSrc1(false, false);
		fLoadSrc2(false, false);
		if (pInst->src1 != reg::Zero && pInst->src2 != reg::Zero)
			gen::Add[I::U64::XOr()];
		break;
	case rv64::Opcode::or_reg:
		fLoadSrc1(false, false);
		fLoadSrc2(false, false);
		if (pInst->src1 != reg::Zero && pInst->src2 != reg::Zero)
			gen::Add[I::U64::Or()];
		break;
	case rv64::Opcode::and_reg:
		fLoadSrc1(false, false);
		fLoadSrc2(false, false);
		gen::Add[I::U64::And()];
		break;
	case rv64::Opcode::mul_reg:
		fLoadSrc1(false, false);
		fLoadSrc2(false, false);
		gen::Add[I::I64::Mul()];
		break;
	case rv64::Opcode::mul_reg_half:
		fLoadSrc1(false, true);
		fLoadSrc2(false, true);
		gen::Add[I::I32::Mul()];
		gen::Add[I::I32::Expand()];
		break;
	case rv64::Opcode::shift_left_logic_reg:
		/* wasm-standard automatically performs masking of value, identical to requirements of riscv */
		if (fLoadSrc1(true, false) && fLoadSrc2(false, false))
			gen::Add[I::U64::ShiftLeft()];
		break;
	case rv64::Opcode::shift_left_logic_reg_half:
		/* wasm-standard automatically performs masking of value, identical to requirements of riscv */
		if (fLoadSrc1(true, true) && fLoadSrc2(false, true))
			gen::Add[I::U32::ShiftLeft()];
		gen::Add[I::I32::Expand()];
		break;
	case rv64::Opcode::shift_right_logic_reg:
		/* wasm-standard automatically performs masking of value, identical to requirements of riscv */
		if (fLoadSrc1(true, false) && fLoadSrc2(false, false))
			gen::Add[I::U64::ShiftRight()];
		break;
	case rv64::Opcode::shift_right_logic_reg_half:
		/* wasm-standard automatically performs masking of value, identical to requirements of riscv */
		if (fLoadSrc1(true, true) && fLoadSrc2(false, true))
			gen::Add[I::U32::ShiftRight()];
		gen::Add[I::I32::Expand()];
		break;
	case rv64::Opcode::shift_right_arith_reg:
		/* wasm-standard automatically performs masking of value, identical to requirements of riscv */
		if (fLoadSrc1(true, false) && fLoadSrc2(false, false))
			gen::Add[I::I64::ShiftRight()];
		break;
	case rv64::Opcode::shift_right_arith_reg_half:
		/* wasm-standard automatically performs masking of value, identical to requirements of riscv */
		if (fLoadSrc1(true, true) && fLoadSrc2(false, true))
			gen::Add[I::I32::ShiftRight()];
		gen::Add[I::I32::Expand()];
		break;
	case rv64::Opcode::set_less_than_s_reg:
		fLoadSrc1(true, false);
		fLoadSrc2(true, false);
		gen::Add[I::I64::Less()];
		gen::Add[I::U32::Expand()];
		break;
	case rv64::Opcode::set_less_than_u_reg:
		fLoadSrc1(true, false);
		fLoadSrc2(true, false);
		gen::Add[I::U64::Less()];
		gen::Add[I::U32::Expand()];
		break;
	default:
		break;
	}

	/* write the result to the register */
	fStoreDest();
}
void rv64::Translate::fMakeLoad(bool multi) const {
	/* check if the operation can be discarded */
	if (pInst->dest == reg::Zero)
		return;

	/* compute the destination address and write it to the stack */
	if (multi)
		gen::Add[I::I64::Const(pAddress + pInst->imm)];
	else {
		gen::Add[I::I64::Const(pInst->imm)];
		if (fLoadSrc1(false, false))
			gen::Add[I::U64::Add()];
	}

	/* perform the actual load of the value (memory-register maps 1-to-1 to memory-cache no matter if read or write) */
	switch (pInst->opcode) {
	case rv64::Opcode::load_byte_s:
	case rv64::Opcode::multi_load_byte_s:
		gen::Make->read(pInst->src1, gen::MemoryType::i8To64, pAddress);
		break;
	case rv64::Opcode::load_half_s:
	case rv64::Opcode::multi_load_half_s:
		gen::Make->read(pInst->src1, gen::MemoryType::i16To64, pAddress);
		break;
	case rv64::Opcode::load_word_s:
	case rv64::Opcode::multi_load_word_s:
		gen::Make->read(pInst->src1, gen::MemoryType::i32To64, pAddress);
		break;
	case rv64::Opcode::load_byte_u:
	case rv64::Opcode::multi_load_byte_u:
		gen::Make->read(pInst->src1, gen::MemoryType::u8To64, pAddress);
		break;
	case rv64::Opcode::load_half_u:
	case rv64::Opcode::multi_load_half_u:
		gen::Make->read(pInst->src1, gen::MemoryType::u16To64, pAddress);
		break;
	case rv64::Opcode::load_word_u:
	case rv64::Opcode::multi_load_word_u:
		gen::Make->read(pInst->src1, gen::MemoryType::u32To64, pAddress);
		break;
	case rv64::Opcode::load_dword:
	case rv64::Opcode::multi_load_dword:
		gen::Make->read(pInst->src1, gen::MemoryType::i64, pAddress);
		break;
	default:
		break;
	}

	/* write the value to the register */
	fStoreDest();
}
void rv64::Translate::fMakeStore(bool multi) const {
	/* compute the destination address and write it to the stack */
	if (multi)
		gen::Add[I::I64::Const(pAddress + pInst->imm)];
	else {
		gen::Add[I::I64::Const(pInst->imm)];
		if (fLoadSrc1(false, false))
			gen::Add[I::U64::Add()];
	}

	/* write the source value to the stack */
	fLoadSrc2(true, (pInst->opcode != rv64::Opcode::store_dword && pInst->opcode != rv64::Opcode::multi_store_dword));

	/* perform the actual store of the value (memory-register maps 1-to-1 to memory-cache no matter if read or write) */
	switch (pInst->opcode) {
	case rv64::Opcode::store_byte:
	case rv64::Opcode::multi_store_byte:
		gen::Make->write(pInst->src1, gen::MemoryType::u8To32, pAddress);
		break;
	case rv64::Opcode::store_half:
	case rv64::Opcode::multi_store_half:
		gen::Make->write(pInst->src1, gen::MemoryType::u16To32, pAddress);
		break;
	case rv64::Opcode::store_word:
	case rv64::Opcode::multi_store_word:
		gen::Make->write(pInst->src1, gen::MemoryType::i32, pAddress);
		break;
	case rv64::Opcode::store_dword:
	case rv64::Opcode::multi_store_dword:
		gen::Make->write(pInst->src1, gen::MemoryType::i64, pAddress);
		break;
	default:
		break;
	}
}
void rv64::Translate::fMakeDivRem() {
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
			gen::Add[I::I64::Const(-1)];
			fStoreDest();
		}
		else if (pInst->src1 != pInst->dest) {
			fLoadSrc1(true, false);
			fStoreDest();
		}
		return;
	}

	/* allocate the temporary variables and write the operands to the stack */
	wasm::Variable temp = (half ? fTemp32(0) : fTemp64(0));
	fLoadSrc1(true, half);
	fLoadSrc2(true, half);

	/* check if a division-by-zero will occur */
	gen::Add[I::Local::Tee(temp)];
	gen::Add[half ? I::U32::EqualZero() : I::U64::EqualZero()];
	wasm::IfThen zero{ gen::Sink, u8"", { type }, {} };

	/* write the division-by-zero result to the stack (for remainder, its just the first operand) */
	if (div) {
		gen::Add[I::Drop()];
		gen::Add[I::I64::Const(-1)];
	}
	fStoreDest();

	/* restore the second operand */
	zero.otherwise();
	gen::Add[I::Local::Get(temp)];

	/* check if a division overflow may occur */
	wasm::Block overflown;
	if (sign) {
		overflown = std::move(wasm::Block{ gen::Sink, u8"", { type, type }, {} });
		gen::Add[half ? I::I32::Const(-1) : I::I64::Const(-1)];
		gen::Add[half ? I::U32::Equal() : I::U64::Equal()];
		wasm::IfThen mayOverflow{ gen::Sink, u8"", { type }, { type, type } };

		/* check the second operand (temp can be overwritten, as the divisor is now known) */
		gen::Add[I::Local::Tee(temp)];
		gen::Add[half ? I::I32::Const(std::numeric_limits<int32_t>::min()) : I::I64::Const(std::numeric_limits<int64_t>::min())];
		gen::Add[half ? I::U32::Equal() : I::U64::Equal()];
		{
			wasm::IfThen isOverflow{ gen::Sink, u8"", {}, { type, type } };

			/* write the overflow result to the destination */
			if (div)
				gen::Add[half ? I::I32::Const(std::numeric_limits<int32_t>::min()) : I::I64::Const(std::numeric_limits<int64_t>::min())];
			else
				gen::Add[half ? I::I32::Const(0) : I::I64::Const(0)];
			fStoreDest();
			gen::Add[I::Branch::Direct(overflown)];

			/* restore the operands for the operation */
			isOverflow.otherwise();
			gen::Add[I::Local::Get(temp)];
			gen::Add[half ? I::I32::Const(-1) : I::I64::Const(-1)];
		}

		/* restore the first operand */
		mayOverflow.otherwise();
		gen::Add[I::Local::Get(temp)];
	}

	/* write the result of the operation to the stack */
	switch (pInst->opcode) {
	case rv64::Opcode::div_s_reg:
		gen::Add[I::I64::Div()];
		break;
	case rv64::Opcode::div_s_reg_half:
		gen::Add[I::I32::Div()];
		gen::Add[I::I32::Expand()];
		break;
	case rv64::Opcode::div_u_reg:
		gen::Add[I::U64::Div()];
		break;
	case rv64::Opcode::div_u_reg_half:
		gen::Add[I::U32::Div()];
		gen::Add[I::I32::Expand()];
		break;
	case rv64::Opcode::rem_s_reg:
		gen::Add[I::I64::Mod()];
		break;
	case rv64::Opcode::rem_s_reg_half:
		gen::Add[I::I32::Mod()];
		gen::Add[I::I32::Expand()];
		break;
	case rv64::Opcode::rem_u_reg:
		gen::Add[I::U64::Mod()];
		break;
	case rv64::Opcode::rem_u_reg_half:
		gen::Add[I::U32::Mod()];
		gen::Add[I::I32::Expand()];
		break;
	default:
		break;
	}

	/* write the result to the register */
	fStoreDest();
}
void rv64::Translate::fMakeAMO(bool half) {
	/*
	*	Note: simply treat the operations as all being atomic, as no threading is supported
	*/

	/* load the source address into the temporary */
	wasm::Variable addr = fTemp64(0);
	fLoadSrc1(true, false);
	gen::Add[I::Local::Tee(addr)];

	/* validate the alignment constraints of the address */
	gen::Add[I::U64::Shrink()];
	gen::Add[I::U32::Const(half ? 0x03 : 0x07)];
	gen::Add[I::U32::And()];
	{
		wasm::IfThen _if{ gen::Sink };
		pWriter->makeException(Translate::MisAlignedException, pAddress, pNextAddress);
	}

	/* perform the reading of the original value (dont write it to the destination yet, as the destination migth also be the source) */
	wasm::Variable value = (half ? fTemp32(0) : fTemp64(1));
	gen::Add[I::Local::Get(addr)];
	gen::Make->read(pInst->src1, (half ? gen::MemoryType::i32 : gen::MemoryType::i64), pAddress);
	gen::Add[I::Local::Set(value)];

	/* write the destination address to the stack */
	gen::Add[I::Local::Get(addr)];

	/* write the new value to the stack (perform the operation in the corresponding width) */
	switch (pInst->opcode) {
	case rv64::Opcode::amo_swap_w:
	case rv64::Opcode::amo_swap_d:
		fLoadSrc2(true, half);
		break;
	case rv64::Opcode::amo_add_w:
	case rv64::Opcode::amo_add_d:
		gen::Add[I::Local::Get(value)];
		if (fLoadSrc2(false, half))
			gen::Add[half ? I::U32::Add() : I::U64::Add()];
		break;
	case rv64::Opcode::amo_xor_w:
	case rv64::Opcode::amo_xor_d:
		gen::Add[I::Local::Get(value)];
		if (fLoadSrc2(false, half))
			gen::Add[half ? I::U32::XOr() : I::U64::XOr()];
		break;
	case rv64::Opcode::amo_and_w:
	case rv64::Opcode::amo_and_d:
		if (fLoadSrc2(true, half)) {
			gen::Add[I::Local::Get(value)];
			gen::Add[half ? I::U32::And() : I::U64::And()];
		}
		break;
	case rv64::Opcode::amo_or_w:
	case rv64::Opcode::amo_or_d:
		gen::Add[I::Local::Get(value)];
		if (fLoadSrc2(false, half))
			gen::Add[half ? I::U32::Or() : I::U64::Or()];
		break;
	case rv64::Opcode::amo_min_s_w:
	case rv64::Opcode::amo_min_s_d:
		if (fLoadSrc2(true, half)) {
			wasm::Variable other = (half ? fTemp32(1) : addr);
			gen::Add[I::Local::Tee(other)];
			gen::Add[I::Local::Get(value)];
			gen::Add[I::Local::Get(other)];
		}
		else {
			gen::Add[I::Local::Get(value)];
			gen::Add[half ? I::U32::Const(0) : I::U64::Const(0)];
		}
		gen::Add[I::Local::Get(value)];
		gen::Add[half ? I::I32::Less() : I::I64::Less()];
		gen::Add[I::Select()];
		break;
	case rv64::Opcode::amo_max_s_w:
	case rv64::Opcode::amo_max_s_d:
		if (fLoadSrc2(true, half)) {
			wasm::Variable other = (half ? fTemp32(1) : addr);
			gen::Add[I::Local::Tee(other)];
			gen::Add[I::Local::Get(value)];
			gen::Add[I::Local::Get(other)];
		}
		else {
			gen::Add[I::Local::Get(value)];
			gen::Add[half ? I::U32::Const(0) : I::U64::Const(0)];
		}
		gen::Add[I::Local::Get(value)];
		gen::Add[half ? I::I32::Greater() : I::I64::Greater()];
		gen::Add[I::Select()];
		break;
	case rv64::Opcode::amo_min_u_w:
	case rv64::Opcode::amo_min_u_d:
		if (fLoadSrc2(true, half)) {
			wasm::Variable other = (half ? fTemp32(1) : addr);
			gen::Add[I::Local::Tee(other)];
			gen::Add[I::Local::Get(value)];
			gen::Add[I::Local::Get(other)];
			gen::Add[I::Local::Get(value)];
			gen::Add[half ? I::U32::Less() : I::U64::Less()];
			gen::Add[I::Select()];
		}
		break;
	case rv64::Opcode::amo_max_u_w:
	case rv64::Opcode::amo_max_u_d:
		if (fLoadSrc2(false, half)) {
			wasm::Variable other = (half ? fTemp32(1) : addr);
			gen::Add[I::Local::Tee(other)];
			gen::Add[I::Local::Get(value)];
			gen::Add[I::Local::Get(other)];
			gen::Add[I::Local::Get(value)];
			gen::Add[half ? I::U32::Greater() : I::U64::Greater()];
			gen::Add[I::Select()];
		}
		else
			gen::Add[I::Local::Get(value)];
		break;
	default:
		break;
	}

	/* write the result back to the memory */
	gen::Make->write(pInst->src1, (half ? gen::MemoryType::i32 : gen::MemoryType::i64), pAddress);

	/* write the original value back to the destination */
	gen::Add[I::Local::Get(value)];
	if (half)
		gen::Add[I::I32::Expand()];
	fStoreDest();
}
void rv64::Translate::fMakeAMOLR() {
	/*
	*	Note: simply treat the operations as all being atomic, as no threading is supported
	*/

	/* operation-checks to simplify the logic */
	bool half = (pInst->opcode == rv64::Opcode::load_reserved_w);

	/* load the source address into the temporary */
	wasm::Variable addr = fTemp64(0);
	fLoadSrc1(true, false);
	gen::Add[I::Local::Tee(addr)];

	/* validate the alignment constraints of the address */
	gen::Add[I::U64::Shrink()];
	gen::Add[I::U32::Const(half ? 0x03 : 0x07)];
	gen::Add[I::U32::And()];
	{
		wasm::IfThen _if{ gen::Sink };
		pWriter->makeException(Translate::MisAlignedException, pAddress, pNextAddress);
	}

	/* write the value from memory to the register */
	gen::Add[I::Local::Get(addr)];
	gen::Make->read(pInst->src1, (half ? gen::MemoryType::i32To64 : gen::MemoryType::i64), pAddress);
	fStoreDest();
}
void rv64::Translate::fMakeAMOSC() {
	/*
	*	Note: simply treat the operations as all being atomic, as no threading is supported
	*/

	/* operation-checks to simplify the logic */
	bool half = (pInst->opcode == rv64::Opcode::store_conditional_w);

	/* load the source address into the temporary */
	wasm::Variable addr = fTemp64(0);
	fLoadSrc1(true, false);
	gen::Add[I::Local::Tee(addr)];

	/* validate the alignment constraints of the address */
	gen::Add[I::U64::Shrink()];
	gen::Add[I::U32::Const(half ? 0x03 : 0x07)];
	gen::Add[I::U32::And()];
	{
		wasm::IfThen _if{ gen::Sink };
		pWriter->makeException(Translate::MisAlignedException, pAddress, pNextAddress);
	}

	/* write the source value to the address (assumption that reservation is always valid) */
	gen::Add[I::Local::Get(addr)];
	fLoadSrc2(true, half);
	gen::Make->write(pInst->src1, (half ? gen::MemoryType::i32 : gen::MemoryType::i64), pAddress);

	/* write the result to the destination register */
	gen::Add[I::U64::Const(0)];
	fStoreDest();
}
void rv64::Translate::fMakeMul() {
	/*
	*	Performed operation:
	*
	*	mul: a0a1 * b0b1 where each component is 32bits
	*
	*	r = a0 * b0
	*	r = (r >> 32) + a0 * b1 + a1 * b0
	*	r = (r >> 32) + a0a1 * b2b3 + a2a3 * b0b1 + a1 * b1
	*/

	/* operation-checks to simplify the logic */
	bool bSigned = (pInst->opcode == rv64::Opcode::mul_high_s_reg);
	bool isSigned = (bSigned || pInst->opcode == rv64::Opcode::mul_high_s_u_reg);

	/* check if the operation can be discarded */
	if (pInst->dest == reg::Zero)
		return;

	/* check if the operation can be short-circuited */
	if (pInst->src1 == reg::Zero || pInst->src2 == reg::Zero) {
		gen::Add[I::U64::Const(0)];
		fStoreDest();
		return;
	}

	/* check if at least one component is signed, in which case the full values need to be loaded before splitting
	*	them into the separate words (b-signed implies a signed as well; the values are loaded into a1/b1) */
	wasm::Variable a0 = fTemp64(0), a1 = fTemp64(1), b0 = fTemp64(2), b1 = fTemp64(3);
	if (isSigned) {
		/* load the two components and leave them on the stack (start with the sign-extension of a) */
		fLoadSrc1(false, false);
		gen::Add[I::Local::Tee(a1)];
		gen::Add[I::I64::Const(63)];
		gen::Add[I::I64::ShiftRight()];
		fLoadSrc2(false, false);
		gen::Add[I::Local::Tee(b1)];

		/* perform the first multiplication [a2a3 * b0b1] */
		gen::Add[I::U64::Mul()];

		/* perform the second multiplication, if b is sign-extended as well [a0a1 * b2b3] */
		if (bSigned) {
			gen::Add[I::Local::Get(b1)];
			gen::Add[I::I64::Const(63)];
			gen::Add[I::I64::ShiftRight()];
			gen::Add[I::Local::Get(a1)];
			gen::Add[I::U64::Mul()];
			gen::Add[I::U64::Add()];
		}
	}

	/* split the two variables into the separate word-components (not sign-extended,
	*	as the extension is handled separately) and leave the upper two words on the stack */
	for (size_t i = 0; i < 2; ++i) {
		wasm::Variable& w0 = (i == 0 ? a0 : b0);
		wasm::Variable& w1 = (i == 0 ? a1 : b1);

		/* load the total value (leave a copy in w1) */
		if (isSigned)
			gen::Add[I::Local::Get(w1)];
		else {
			if (i == 0)
				fLoadSrc1(false, false);
			else
				fLoadSrc2(false, false);
			gen::Add[I::Local::Tee(w1)];
		}

		/* write the lower word back */
		gen::Add[I::U64::Const(0xffff'ffff)];
		gen::Add[I::U64::And()];
		gen::Add[I::Local::Set(w0)];

		/* write the upper word back, but leave a copy of it on the stack */
		gen::Add[I::Local::Get(w1)];
		gen::Add[I::U64::Const(32)];
		gen::Add[I::U64::ShiftRight()];
		gen::Add[I::Local::Tee(w1)];
	}

	/* perform the multiplication of the two upper words [a1 * b1] and add it to the overall result */
	gen::Add[I::U64::Mul()];
	if (isSigned)
		gen::Add[I::U64::Add()];

	/* perform the multiplication of the lower two words, and shift it [(a0 * b0) >> 32] */
	gen::Add[I::Local::Get(a0)];
	gen::Add[I::Local::Get(b0)];
	gen::Add[I::U64::Mul()];
	gen::Add[I::U64::Const(32)];
	gen::Add[I::U64::ShiftRight()];

	/* perfrom the cross multiplication, add the lower multiplication to it [((a0 * b0) >> 32) + a0 * b1 + b0 * a1] */
	gen::Add[I::Local::Get(a0)];
	gen::Add[I::Local::Get(b1)];
	gen::Add[I::U64::Mul()];
	gen::Add[I::U64::Add()];
	gen::Add[I::Local::Get(a1)];
	gen::Add[I::Local::Get(b0)];
	gen::Add[I::U64::Mul()];
	gen::Add[I::U64::Add()];

	/* shift the last stage and add it to the overal result */
	gen::Add[I::U64::Const(32)];
	gen::Add[I::U64::ShiftRight()];
	gen::Add[I::U64::Add()];

	/* write the result to the register */
	fStoreDest();
}

void rv64::Translate::fMakeFLoad(bool multi) const {
	/* compute the destination address and write it to the stack */
	if (multi)
		gen::Add[I::I64::Const(pAddress + pInst->imm)];
	else {
		gen::Add[I::I64::Const(pInst->imm)];
		if (fLoadSrc1(false, false))
			gen::Add[I::U64::Add()];
	}

	/* perform the actual load of the value (memory-register maps 1-to-1 to memory-cache no matter if read or write) */
	switch (pInst->opcode) {
	case rv64::Opcode::load_float:
	case rv64::Opcode::multi_load_float:
		gen::Make->read(pInst->src1, gen::MemoryType::i32, pAddress);
		fExpandFloat(true, true);
		break;
	case rv64::Opcode::load_double:
	case rv64::Opcode::multi_load_double:
		gen::Make->read(pInst->src1, gen::MemoryType::i64, pAddress);
		break;
	default:
		break;
	}

	/* write the value to the register */
	fStoreFDest(true);
}
void rv64::Translate::fMakeFStore(bool multi) const {
	/* compute the destination address and write it to the stack */
	if (multi)
		gen::Add[I::I64::Const(pAddress + pInst->imm)];
	else {
		gen::Add[I::I64::Const(pInst->imm)];
		if (fLoadSrc1(false, false))
			gen::Add[I::U64::Add()];
	}

	/* write the source value to the stack */
	fLoadFSrc2(pInst->opcode == rv64::Opcode::store_float || pInst->opcode == rv64::Opcode::multi_store_float);

	/* perform the actual store of the value (memory-register maps 1-to-1 to memory-cache no matter if read or write) */
	switch (pInst->opcode) {
	case rv64::Opcode::store_float:
	case rv64::Opcode::multi_store_float:
		gen::Make->write(pInst->src1, gen::MemoryType::f32, pAddress);
		break;
	case rv64::Opcode::store_double:
	case rv64::Opcode::multi_store_double:
		gen::Make->write(pInst->src1, gen::MemoryType::f64, pAddress);
		break;
	default:
		break;
	}
}

void rv64::Translate::resetAll(sys::Writer* writer) {
	for (wasm::Variable& var : pTemp)
		var = wasm::Variable{};
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

	/* check if a comment should be added */
	if (env::Instance()->logBlocks())
		gen::Sink->comment(str::u8::Build(str::As{ U"#018x", pAddress }, u8": ", rv64::ToString(inst)));

	/* perform the actual translation */
	switch (pInst->opcode) {
	case rv64::Opcode::misaligned:
		pWriter->makeException(Translate::MisAlignedException, pAddress, pNextAddress);
		break;
	case rv64::Opcode::nop:
		gen::Add[I::Nop()];
		break;
	case rv64::Opcode::load_upper_imm:
	case rv64::Opcode::add_upper_imm_pc:
	case rv64::Opcode::multi_load_imm:
	case rv64::Opcode::multi_load_address:
		fMakeImms();
		break;
	case rv64::Opcode::jump_and_link_imm:
	case rv64::Opcode::multi_call:
	case rv64::Opcode::multi_tail:
		fMakeJALI();
		break;
	case rv64::Opcode::jump_and_link_reg:
		fMakeJALR();
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
	case rv64::Opcode::mul_reg:
	case rv64::Opcode::mul_reg_half:
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
		fMakeLoad(false);
		break;
	case rv64::Opcode::store_byte:
	case rv64::Opcode::store_half:
	case rv64::Opcode::store_word:
	case rv64::Opcode::store_dword:
		fMakeStore(false);
		break;
	case rv64::Opcode::multi_load_byte_s:
	case rv64::Opcode::multi_load_half_s:
	case rv64::Opcode::multi_load_word_s:
	case rv64::Opcode::multi_load_byte_u:
	case rv64::Opcode::multi_load_half_u:
	case rv64::Opcode::multi_load_word_u:
	case rv64::Opcode::multi_load_dword:
		fMakeLoad(true);
		break;
	case rv64::Opcode::multi_store_byte:
	case rv64::Opcode::multi_store_half:
	case rv64::Opcode::multi_store_word:
	case rv64::Opcode::multi_store_dword:
		fMakeStore(true);
		break;
	case rv64::Opcode::ecall:
		pWriter->makeSyscall(pAddress, pNextAddress);
		break;
	case rv64::Opcode::ebreak:
		pWriter->makeException(Translate::EBreakException, pAddress, pNextAddress);
		break;
	case rv64::Opcode::fence:
		pWriter->makeFlushMemCache(pAddress, pNextAddress);
		break;
	case rv64::Opcode::fence_inst:
		pWriter->makeFlushInstCache(pAddress, pNextAddress);
		break;
	case rv64::Opcode::mul_high_s_reg:
	case rv64::Opcode::mul_high_s_u_reg:
	case rv64::Opcode::mul_high_u_reg:
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
	case rv64::Opcode::load_float:
	case rv64::Opcode::load_double:
		fMakeFLoad(false);
		break;
	case rv64::Opcode::store_float:
	case rv64::Opcode::store_double:
		fMakeFStore(false);
		break;
	case rv64::Opcode::multi_load_float:
	case rv64::Opcode::multi_load_double:
		fMakeFLoad(true);
		break;
	case rv64::Opcode::multi_store_float:
	case rv64::Opcode::multi_store_double:
		fMakeFStore(true);
		break;
	case rv64::Opcode::csr_read_write:
	case rv64::Opcode::csr_read_and_set:
	case rv64::Opcode::csr_read_and_clear:
	case rv64::Opcode::csr_read_write_imm:
	case rv64::Opcode::csr_read_and_set_imm:
	case rv64::Opcode::csr_read_and_clear_imm:
	case rv64::Opcode::float_mul_add:
	case rv64::Opcode::float_mul_sub:
	case rv64::Opcode::float_neg_mul_add:
	case rv64::Opcode::float_neg_mul_sub:
	case rv64::Opcode::float_add:
	case rv64::Opcode::float_sub:
	case rv64::Opcode::float_mul:
	case rv64::Opcode::float_div:
	case rv64::Opcode::float_sqrt:
	case rv64::Opcode::float_sign_copy:
	case rv64::Opcode::float_sign_invert:
	case rv64::Opcode::float_sign_xor:
	case rv64::Opcode::float_min:
	case rv64::Opcode::float_max:
	case rv64::Opcode::float_less_equal:
	case rv64::Opcode::float_less_than:
	case rv64::Opcode::float_equal:
	case rv64::Opcode::float_classify:
	case rv64::Opcode::double_mul_add:
	case rv64::Opcode::double_mul_sub:
	case rv64::Opcode::double_neg_mul_add:
	case rv64::Opcode::double_neg_mul_sub:
	case rv64::Opcode::double_add:
	case rv64::Opcode::double_sub:
	case rv64::Opcode::double_mul:
	case rv64::Opcode::double_div:
	case rv64::Opcode::double_sqrt:
	case rv64::Opcode::double_sign_copy:
	case rv64::Opcode::double_sign_invert:
	case rv64::Opcode::double_sign_xor:
	case rv64::Opcode::double_min:
	case rv64::Opcode::double_max:
	case rv64::Opcode::double_less_equal:
	case rv64::Opcode::double_less_than:
	case rv64::Opcode::double_equal:
	case rv64::Opcode::double_classify:
	case rv64::Opcode::float_convert_to_word_s:
	case rv64::Opcode::float_convert_to_word_u:
	case rv64::Opcode::float_convert_to_dword_s:
	case rv64::Opcode::float_convert_to_dword_u:
	case rv64::Opcode::double_convert_to_word_s:
	case rv64::Opcode::double_convert_to_word_u:
	case rv64::Opcode::double_convert_to_dword_s:
	case rv64::Opcode::double_convert_to_dword_u:
	case rv64::Opcode::float_convert_from_word_s:
	case rv64::Opcode::float_convert_from_word_u:
	case rv64::Opcode::float_convert_from_dword_s:
	case rv64::Opcode::float_convert_from_dword_u:
	case rv64::Opcode::double_convert_from_word_s:
	case rv64::Opcode::double_convert_from_word_u:
	case rv64::Opcode::double_convert_from_dword_s:
	case rv64::Opcode::double_convert_from_dword_u:
	case rv64::Opcode::float_move_to_word:
	case rv64::Opcode::float_move_from_word:
	case rv64::Opcode::double_move_to_dword:
	case rv64::Opcode::double_move_from_dword:
	case rv64::Opcode::float_to_double:
	case rv64::Opcode::double_to_float:
	case rv64::Opcode::_invalid:
		/* raise the not-implemented exception for all remaining instructions */
		pWriter->makeException(Translate::NotImplException, pAddress, pNextAddress);
		break;
	}
}
