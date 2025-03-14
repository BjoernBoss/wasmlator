/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025 Bjoern Boss Henrichsen */
#include "rv64-translation.h"
#include "rv64-print.h"

static util::Logger logger{ u8"rv64::cpu" };

std::pair<uint32_t, uint32_t> rv64::Translate::fGetCsrPlacement(uint16_t csr) const {
	uint32_t shift = 0, mask = 0;

	switch (csr) {
	case csr::fpExceptionFlags:
		shift = 0;
		mask = 0x1f;
		break;
	case csr::fpRoundingMode:
		shift = 5;
		mask = 0x07;
		break;
	case csr::fpStatus:
		shift = 0;
		mask = 0xff;
		break;
	}

	return { shift, mask };
}
wasm::Variable rv64::Translate::fTempi32(size_t index) {
	wasm::Variable& var = pTemp[4 + index];
	if (!var.valid())
		var = gen::Sink->local(wasm::Type::i32, str::u8::Build(u8"_temp_i32_", index));
	return var;
}
wasm::Variable rv64::Translate::fTempi64(size_t index) {
	wasm::Variable& var = pTemp[index];
	if (!var.valid())
		var = gen::Sink->local(wasm::Type::i64, str::u8::Build(u8"_temp_i64_", index));
	return var;
}
wasm::Variable rv64::Translate::fTempf32() {
	wasm::Variable& var = pTemp[6];
	if (!var.valid())
		var = gen::Sink->local(wasm::Type::f32, str::u8::Build(u8"_temp_f32"));
	return var;
}
wasm::Variable rv64::Translate::fTempf64() {
	wasm::Variable& var = pTemp[7];
	if (!var.valid())
		var = gen::Sink->local(wasm::Type::f64, str::u8::Build(u8"_temp_f64"));
	return var;
}

uint32_t rv64::Translate::fClassifyFloat(int _class, bool _sign, bool _topMantissa) {
	uint32_t out = 0;
	auto set = [&](uint32_t index, bool value) { out |= ((value ? 0x01 : 0x00) << index); };

	/*
	*	bit[0]: is -inf
	*	bit[1]: is neg normal
	*	bit[2]: is neg subnormal
	*	bit[3]: is -0
	*	bit[4]: is +0
	*	bit[5]: is pos subnormal number
	*	bit[6]: is pos normal number
	*	bit[7]: is +inf
	*	bit[8]: is signaling NaN
	*	bit[9]: is quite NaN
	*/

	switch (_class) {
	case FP_INFINITE:
		set(0, _sign);
		set(7, !_sign);
		break;
	case FP_NORMAL:
		set(1, _sign);
		set(6, !_sign);
		break;
	case FP_SUBNORMAL:
		set(2, _sign);
		set(5, !_sign);
		break;
	case FP_ZERO:
		set(3, _sign);
		set(4, !_sign);
		break;
	case FP_NAN:
		set(8, !_topMantissa);
		set(9, _topMantissa);
		break;
	}
	return out;
}
uint32_t rv64::Translate::fClassifyf32(float value) {
	int _class = std::fpclassify(value);
	bool _sign = std::signbit(value);
	bool _topMantissa = bool((std::bit_cast<uint32_t, float>(value) >> 22) & 0x01);
	return fClassifyFloat(_class, _sign, _topMantissa);
}
uint32_t rv64::Translate::fClassifyf64(double value) {
	int _class = std::fpclassify(value);
	bool _sign = std::signbit(value);
	bool _topMantissa = bool((std::bit_cast<uint64_t, double>(value) >> 51) & 0x01);
	return fClassifyFloat(_class, _sign, _topMantissa);
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
gen::FulFill rv64::Translate::fStoreReg(uint8_t reg) const {
	if (reg != reg::Zero)
		return gen::Make->set(offsetof(rv64::Context, iregs) + reg * sizeof(uint64_t), gen::MemoryType::i64);
	return gen::FulFill{};
}
gen::FulFill rv64::Translate::fStoreDest() const {
	return fStoreReg(pInst->dest);
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
gen::FulFill rv64::Translate::fStoreFDest(bool isAsInt) const {
	if (isAsInt)
		return gen::Make->set(offsetof(rv64::Context, fregs) + pInst->dest * sizeof(double), gen::MemoryType::i64);
	return gen::Make->set(offsetof(rv64::Context, fregs) + pInst->dest * sizeof(double), gen::MemoryType::f64);
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

	/* prepare the result writeback */
	gen::FulFill fulfill = fStoreDest();

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
	fulfill.now();
}
void rv64::Translate::fMakeJALI() {
	/* write the next pc to the destination register */
	if (pInst->opcode == rv64::Opcode::multi_call) {
		gen::FulFill fulfill = fStoreReg(reg::X1);
		gen::Add[I::U64::Const(pNextAddress)];
		fulfill.now();
	}
	else if (pInst->opcode == rv64::Opcode::jump_and_link_imm && pInst->dest != reg::Zero) {
		gen::FulFill fulfill = fStoreDest();
		gen::Add[I::U64::Const(pNextAddress)];
		fulfill.now();
	}

	/* check if the target is misaligned and add the misalignment-exception */
	env::guest_t address = (pAddress + pInst->imm);
	if (pInst->isMisaligned(pAddress))
		pWriter->makeException(Translate::MisalignedException, pAddress, pNextAddress);

	/* add the instruction as normal direct call or jump */
	else if (pInst->isCall())
		gen::Make->call(address, pNextAddress);
	else
		gen::Make->jump(address);
}
void rv64::Translate::fMakeJALR() {
	/* write the target address to the stack */
	if (fLoadSrc1(false, false)) {
		if (pInst->imm != 0) {
			gen::Add[I::I64::Const(pInst->imm)];
			gen::Add[I::I64::Add()];
		}
	}
	else
		gen::Add[I::I64::Const(pInst->imm)];

	/* write the next pc to the destination register */
	if (pInst->dest != reg::Zero) {
		gen::FulFill fulfill = fStoreDest();
		gen::Add[I::U64::Const(pNextAddress)];
		fulfill.now();
	}

	/* perform the alignment validation */
	wasm::Variable addr = fTempi64(0);
	gen::Add[I::Local::Tee(addr)];
	gen::Add[I::U64::Shrink()];
	gen::Add[I::U32::Const(1)];
	gen::Add[I::U32::And()];
	{
		wasm::IfThen _if{ gen::Sink };
		pWriter->makeException(Translate::MisalignedException, pAddress, pNextAddress);
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
	if (pInst->isMisaligned(pAddress)) {
		pWriter->makeException(Translate::MisalignedException, pAddress, pNextAddress);
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

	/* prepare the result writeback */
	gen::FulFill fulfill = fStoreDest();

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
	fulfill.now();
}
void rv64::Translate::fMakeALUReg() const {
	/* check if the operation can be discarded */
	if (pInst->dest == reg::Zero)
		return;

	/* prepare the result writeback */
	gen::FulFill fulfill = fStoreDest();

	/* check if the two sources are null, in which case nothing needs to be done */
	if (pInst->src1 == reg::Zero && pInst->src2 == reg::Zero) {
		gen::Add[I::U64::Const(0)];
		fulfill.now();
		return;
	}

	/* check if the operation can be short-circuited */
	if ((pInst->src1 == reg::Zero || pInst->src2 == reg::Zero) && (pInst->opcode == rv64::Opcode::and_reg ||
		pInst->opcode == rv64::Opcode::mul_reg || pInst->opcode == rv64::Opcode::mul_reg_half)) {
		gen::Add[I::U64::Const(0)];
		fulfill.now();
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
	fulfill.now();
}
void rv64::Translate::fMakeLoad(bool multi) const {
	/* check if the operation can be discarded */
	if (pInst->dest == reg::Zero)
		return;

	/* prepare the result writeback */
	gen::FulFill fulfill = fStoreDest();

	/* compute the destination address and write it to the stack */
	if (multi)
		gen::Add[I::I64::Const(pAddress + pInst->imm)];
	else if (pInst->imm != 0) {
		gen::Add[I::I64::Const(pInst->imm)];
		if (fLoadSrc1(false, false))
			gen::Add[I::U64::Add()];
	}
	else
		fLoadSrc1(true, false);

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
	fulfill.now();
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

	/* check if its a mutli-operation, which requires the intermediate register to be set accordingly (will never be zero) */
	if (pInst->opcode == rv64::Opcode::multi_store_byte || pInst->opcode == rv64::Opcode::multi_store_half ||
		pInst->opcode == rv64::Opcode::multi_store_word || pInst->opcode == rv64::Opcode::multi_store_dword) {
		gen::FulFill fulfill = fStoreReg(pInst->src1);
		gen::Add[I::I64::Const(pAddress + pInst->tempValue)];
		fulfill.now();
	}

	/* write the value to memory (memory-register maps 1-to-1 to memory-cache no matter if read or write) */
	switch (pInst->opcode) {
	case rv64::Opcode::store_byte:
	case rv64::Opcode::multi_store_byte:
		gen::Make->write(pInst->src1, gen::MemoryType::u8To32, pAddress, pNextAddress);
		break;
	case rv64::Opcode::store_half:
	case rv64::Opcode::multi_store_half:
		gen::Make->write(pInst->src1, gen::MemoryType::u16To32, pAddress, pNextAddress);
		break;
	case rv64::Opcode::store_word:
	case rv64::Opcode::multi_store_word:
		gen::Make->write(pInst->src1, gen::MemoryType::i32, pAddress, pNextAddress);
		break;
	case rv64::Opcode::store_dword:
	case rv64::Opcode::multi_store_dword:
		gen::Make->write(pInst->src1, gen::MemoryType::i64, pAddress, pNextAddress);
		break;
	default:
		break;
	}
}
void rv64::Translate::fMakeDivRem() {
	/* check if the operation can be discarded */
	if (pInst->dest == reg::Zero)
		return;

	/* operation-checks to simplify the logic */
	bool half = (pInst->opcode == rv64::Opcode::div_s_reg_half || pInst->opcode == rv64::Opcode::div_u_reg_half ||
		pInst->opcode == rv64::Opcode::rem_s_reg_half || pInst->opcode == rv64::Opcode::rem_u_reg_half);
	bool div = (pInst->opcode == rv64::Opcode::div_s_reg_half || pInst->opcode == rv64::Opcode::div_u_reg_half ||
		pInst->opcode == rv64::Opcode::div_s_reg || pInst->opcode == rv64::Opcode::div_u_reg);
	bool sign = (pInst->opcode == rv64::Opcode::div_s_reg || pInst->opcode == rv64::Opcode::div_s_reg_half ||
		pInst->opcode == rv64::Opcode::rem_s_reg || pInst->opcode == rv64::Opcode::rem_s_reg_half);
	wasm::Type type = (half ? wasm::Type::i32 : wasm::Type::i64);

	/* prepare the result writeback */
	gen::FulFill fulfill = fStoreDest();

	/* check if the operation can be discarded, as it is a division by zero */
	if (pInst->src2 == reg::Zero) {
		if (div) {
			gen::Add[I::I64::Const(-1)];
			fulfill.now();
		}
		else if (pInst->src1 != pInst->dest) {
			fLoadSrc1(true, false);
			fulfill.now();
		}
		return;
	}

	/* allocate the temporary variables and write the operands to the stack */
	wasm::Variable temp = (half ? fTempi32(0) : fTempi64(0));
	fLoadSrc1(true, half);
	fLoadSrc2(true, half);

	/* check if a division-by-zero will occur */
	gen::Add[I::Local::Tee(temp)];
	gen::Add[half ? I::U32::EqualZero() : I::U64::EqualZero()];
	wasm::IfThen zero{ gen::Sink, u8"", { wasm::Type::i32, type }, {} };

	/* write the division-by-zero result to the stack (for remainder, its just the first operand) */
	if (div) {
		gen::Add[I::Drop()];
		gen::Add[I::I64::Const(-1)];
	}
	else if (half)
		gen::Add[I::I32::Expand()];
	fulfill.now();

	/* restore the second operand */
	zero.otherwise();
	gen::Add[I::Local::Get(temp)];

	/* check if a division overflow may occur */
	wasm::Block overflown;
	if (sign) {
		overflown = std::move(wasm::Block{ gen::Sink, u8"", { wasm::Type::i32, type, type }, {} });
		gen::Add[half ? I::I32::Const(-1) : I::I64::Const(-1)];
		gen::Add[half ? I::U32::Equal() : I::U64::Equal()];
		wasm::IfThen mayOverflow{ gen::Sink, u8"", { wasm::Type::i32, type }, { wasm::Type::i32, type, type } };

		/* check the second operand (temp can be overwritten, as the divisor is now known) */
		gen::Add[I::Local::Tee(temp)];
		gen::Add[half ? I::I32::Const(std::numeric_limits<int32_t>::min()) : I::I64::Const(std::numeric_limits<int64_t>::min())];
		gen::Add[half ? I::U32::Equal() : I::U64::Equal()];
		{
			wasm::IfThen isOverflow{ gen::Sink, u8"", { wasm::Type::i32 }, { wasm::Type::i32, type, type } };

			/* write the overflow result to the destination */
			if (div)
				gen::Add[I::I64::Const(std::numeric_limits<int64_t>::min())];
			else
				gen::Add[I::I64::Const(0)];
			fulfill.now();
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
	fulfill.now();
}
void rv64::Translate::fMakeAMO(bool half) {
	/*
	*	Note: simply treat the operations as all being atomic, as no threading is supported
	*/
	gen::MemoryType type = (half ? gen::MemoryType::i32 : gen::MemoryType::i64);

	/* load the source address into the temporary */
	wasm::Variable addr = fTempi64(0);
	fLoadSrc1(true, false);
	gen::Add[I::Local::Tee(addr)];

	/* validate the alignment constraints of the address */
	gen::Add[I::U64::Shrink()];
	gen::Add[I::U32::Const(half ? 0x03 : 0x07)];
	gen::Add[I::U32::And()];
	{
		wasm::IfThen _if{ gen::Sink };
		pWriter->makeException(Translate::MisalignedException, pAddress, pNextAddress);
	}

	/* perform the reading of the original value (dont write it to the destination yet, as the destination migth also be the source) */
	wasm::Variable value = (half ? fTempi32(0) : fTempi64(1));
	gen::Add[I::Local::Get(addr)];
	gen::Make->read(pInst->src1, type, pAddress);
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
			wasm::Variable other = (half ? fTempi32(1) : addr);
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
			wasm::Variable other = (half ? fTempi32(1) : addr);
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
			wasm::Variable other = (half ? fTempi32(1) : addr);
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
			wasm::Variable other = (half ? fTempi32(1) : addr);
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

	/* write the result to the memory */
	gen::Make->write(pInst->src1, type, pAddress, pNextAddress);

	/* write the original value back to the destination */
	if (pInst->dest != reg::Zero) {
		gen::FulFill fulfill = fStoreDest();
		gen::Add[I::Local::Get(value)];
		if (half)
			gen::Add[I::I32::Expand()];
		fulfill.now();
	}
}
void rv64::Translate::fMakeAMOLR() {
	/*
	*	Note: simply treat the operations as all being atomic, as no threading is supported
	*/

	/* operation-checks to simplify the logic */
	bool half = (pInst->opcode == rv64::Opcode::load_reserved_w);

	/* load the source address into the temporary */
	wasm::Variable addr;
	fLoadSrc1(true, false);
	if (pInst->dest != reg::Zero)
		gen::Add[I::Local::Tee(addr = fTempi64(0))];

	/* validate the alignment constraints of the address */
	gen::Add[I::U64::Shrink()];
	gen::Add[I::U32::Const(half ? 0x03 : 0x07)];
	gen::Add[I::U32::And()];
	{
		wasm::IfThen _if{ gen::Sink };
		pWriter->makeException(Translate::MisalignedException, pAddress, pNextAddress);
	}

	/* write the value from memory to the register */
	if (pInst->dest != reg::Zero) {
		gen::FulFill fulfill = fStoreDest();
		gen::Add[I::Local::Get(addr)];
		gen::Make->read(pInst->src1, (half ? gen::MemoryType::i32To64 : gen::MemoryType::i64), pAddress);
		fulfill.now();
	}
}
void rv64::Translate::fMakeAMOSC() {
	/*
	*	Note: simply treat the operations as all being atomic, as no threading is supported
	*/

	/* operation-checks to simplify the logic */
	bool half = (pInst->opcode == rv64::Opcode::store_conditional_w);

	/* load the source address into the temporary */
	wasm::Variable addr = fTempi64(0);
	fLoadSrc1(true, false);
	gen::Add[I::Local::Tee(addr)];

	/* validate the alignment constraints of the address */
	gen::Add[I::U64::Shrink()];
	gen::Add[I::U32::Const(half ? 0x03 : 0x07)];
	gen::Add[I::U32::And()];
	{
		wasm::IfThen _if{ gen::Sink };
		pWriter->makeException(Translate::MisalignedException, pAddress, pNextAddress);
	}

	/* write the source value to the address (assumption that reservation is always valid) */
	gen::Add[I::Local::Get(addr)];
	fLoadSrc2(true, half);
	gen::Make->write(pInst->src1, (half ? gen::MemoryType::i32 : gen::MemoryType::i64), pAddress, pNextAddress);

	/* write the result to the destination register */
	if (pInst->dest != reg::Zero) {
		gen::FulFill fulfill = fStoreDest();
		gen::Add[I::U64::Const(0)];
		fulfill.now();
	}
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

	/* prepare the result writeback */
	gen::FulFill fulfill = fStoreDest();

	/* check if the operation can be short-circuited */
	if (pInst->src1 == reg::Zero || pInst->src2 == reg::Zero) {
		gen::Add[I::U64::Const(0)];
		fulfill.now();
		return;
	}

	/* check if at least one component is signed, in which case the full values need to be loaded before splitting
	*	them into the separate words (b-signed implies a signed as well; the values are loaded into a1/b1) */
	wasm::Variable a0 = fTempi64(0), a1 = fTempi64(1), b0 = fTempi64(2), b1 = fTempi64(3);
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
	fulfill.now();
}
void rv64::Translate::fMakeCSR() {
	/*
	*	Note:
	*		reading a value: read the value prior to executing the instruction
	*		writing a value: write the value after executing the instruction
	*
	*	Currently only float csrs are supported
	*		=> Raise Translate::CsrUnsupported
	*
	*	Currently only the current operations are supported for float csrs
	*		- float-csr will return last set value but only "round to nearest, ties to even" is implemented
	*		- float-flags read will only return 0 or last write
	*/

	/* fetch the properties about the csr value */
	bool readOnly = false, notImplemented = false, nonExisting = false;
	switch (pInst->misc) {
	case csr::fpExceptionFlags:
	case csr::fpRoundingMode:
	case csr::fpStatus:
		readOnly = false;
		break;
	case csr::realTime:
	case csr::instRetired:
	case csr::cycles:
		readOnly = true;
		notImplemented = true;
		break;
	default:
		nonExisting = true;
		return;
	}

	/* check if the instruction performs reads/writes of data */
	bool read = true, write = true;
	switch (pInst->opcode) {
	case rv64::Opcode::csr_read_write:
	case rv64::Opcode::csr_read_write_imm:
		read = (pInst->dest != reg::Zero);
		break;
	case rv64::Opcode::csr_read_and_set:
	case rv64::Opcode::csr_read_and_clear:
		write = (pInst->src1 != reg::Zero);
		break;
	case rv64::Opcode::csr_read_and_set_imm:
	case rv64::Opcode::csr_read_and_clear_imm:
		write = (pInst->imm != reg::Zero);
		break;
	default:
		break;
	}

	/* check if the instruction tries to read/write an undefined csr or tries to write to a read-only csr */
	if (((read || write) && nonExisting) || (write && readOnly)) {
		pWriter->makeException(Translate::IllegalException, pAddress, pNextAddress);
		return;
	}

	/* check if the csr is not implemented (currently only support for fp-status-register) */
	if (notImplemented) {
		pWriter->makeException(Translate::CsrUnsupported, pAddress, pNextAddress);
		return;
	}

	/* check if nothing actually needs to be done */
	read = (read && pInst->dest != reg::Zero);
	if (!read && !write)
		return;

	/* fetch the shift and mask properties of the actual csr */
	auto [shift, mask] = fGetCsrPlacement(pInst->misc);

	/* check if the float read operation is only partially or not implemented */
	if (read && pInst->misc != csr::fpRoundingMode)
		gen::Make->invokeVoid(pRegistered.readFloatCsrWarn);

	/* prepare the result writebacks */
	gen::FulFill csrFulfill = (write ? gen::Make->set(offsetof(rv64::Context, float_csr), gen::MemoryType::i64) : gen::FulFill{});
	gen::FulFill regFulfill = (read ? fStoreDest() : gen::FulFill{});

	/* read the float status-register value to the stack */
	gen::Make->get(offsetof(rv64::Context, float_csr), gen::MemoryType::i64);

	/* check if the original value should be written to the destination register */
	if (read) {
		wasm::Variable t0;
		if (write)
			gen::Add[I::Local::Tee(t0 = fTempi64(0))];

		/* apply the necessary shifting/clipping */
		if (shift > 0) {
			gen::Add[I::U64::Const(5)];
			gen::Add[I::U64::ShiftRight()];
		}
		gen::Add[I::U64::Const(mask)];
		gen::Add[I::U64::And()];

		/* write the value to the destination register */
		regFulfill.now();

		/* restore the original value */
		if (!write)
			return;
		gen::Add[I::Local::Get(t0)];
	}

	/* apply the instruction effect (src register/imm will not be null - as write would otherwise be false) */
	switch (pInst->opcode) {
	case rv64::Opcode::csr_read_and_set:
		fLoadSrc1(false, false);
		gen::Add[I::U64::Const(mask)];
		gen::Add[I::U64::And()];
		if (shift > 0) {
			gen::Add[I::U64::Const(shift)];
			gen::Add[I::U64::ShiftLeft()];
		}
		gen::Add[I::U64::Or()];
		break;
	case rv64::Opcode::csr_read_and_clear:
		fLoadSrc1(false, false);
		gen::Add[I::U64::Const(mask)];
		gen::Add[I::U64::And()];
		if (shift > 0) {
			gen::Add[I::U64::Const(shift)];
			gen::Add[I::U64::ShiftLeft()];
		}
		gen::Add[I::I64::Const(-1)];
		gen::Add[I::U64::XOr()];
		gen::Add[I::U64::And()];
		break;
	case rv64::Opcode::csr_read_and_set_imm:
		gen::Add[I::U64::Const((pInst->imm & mask) << shift)];
		gen::Add[I::U64::Or()];
		break;
	case rv64::Opcode::csr_read_and_clear_imm:
		gen::Add[I::U64::Const(~((pInst->imm & mask) << shift))];
		gen::Add[I::U64::And()];
		break;
	case rv64::Opcode::csr_read_write:
		/* mask out the old bits from the current value */
		gen::Add[I::U64::Const(~(mask << shift))];
		gen::Add[I::U64::And()];

		/* load the new value to be written out and merge the values */
		if (fLoadSrc1(false, false)) {
			gen::Add[I::U64::Const(mask)];
			gen::Add[I::U64::And()];
			if (shift > 0) {
				gen::Add[I::U64::Const(shift)];
				gen::Add[I::U64::ShiftLeft()];
			}
			gen::Add[I::U64::Or()];
		}
		break;
	case rv64::Opcode::csr_read_write_imm:
		/* mask out the old bits from the current value */
		gen::Add[I::U64::Const(~(mask << shift))];
		gen::Add[I::U64::And()];

		/* load the new value to be written out and merge the values */
		if (pInst->imm != 0) {
			gen::Add[I::U64::Const((pInst->imm & mask) << shift)];
			gen::Add[I::U64::Or()];
		}
		break;
	default:
		break;
	}

	/* cache the current resulting value (twice on the stack to ensure temp can be
	*	overwritten again and the original value still resides on the stack) */
	wasm::Variable temp = fTempi64((read && write) ? 1 : 0);
	gen::Add[I::Local::Tee(temp)];
	gen::Add[I::Local::Get(temp)];

	/* extract the rounding mode from the value to be written and cache it to temp */
	auto [frmShift, frmMask] = fGetCsrPlacement(csr::fpRoundingMode);
	gen::Add[I::U64::Const(frmShift)];
	gen::Add[I::U64::ShiftRight()];
	gen::Add[I::U64::Const(frmMask)];
	gen::Add[I::U64::And()];
	gen::Add[I::Local::Tee(temp)];

	/* check if the mode is equal to the supported mode and otherwise warn about the unsupported mode */
	gen::Add[I::U64::Const(frm::roundNearestTiesToEven)];
	gen::Add[I::U64::NotEqual()];
	{
		wasm::IfThen _if{ gen::Sink };
		gen::Add[I::Local::Get(temp)];
		gen::Make->invokeParam(pRegistered.frmFloatCsrWarn);
		gen::Add[I::Drop()];
	}

	/* write the value to the float status-register */
	csrFulfill.now();
}

void rv64::Translate::fMakeFLoad(bool multi) const {
	/* prepare the result writeback */
	gen::FulFill fulfill = fStoreFDest(true);

	/* compute the destination address and write it to the stack */
	if (multi)
		gen::Add[I::I64::Const(pAddress + pInst->imm)];
	else if (pInst->imm != 0) {
		gen::Add[I::I64::Const(pInst->imm)];
		if (fLoadSrc1(false, false))
			gen::Add[I::U64::Add()];
	}
	else
		fLoadSrc1(true, false);

	/* check if its a mutli-operation, which requires the intermediate register to be set accordingly (will never be zero) */
	if (pInst->opcode == rv64::Opcode::multi_load_float || pInst->opcode == rv64::Opcode::multi_load_double) {
		gen::FulFill fulfill = fStoreReg(pInst->src1);
		gen::Add[I::I64::Const(pAddress + pInst->tempValue)];
		fulfill.now();
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
	fulfill.now();
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

	/* check if its a mutli-operation, which requires the intermediate register to be set accordingly (will never be zero) */
	if (pInst->opcode == rv64::Opcode::multi_store_float || pInst->opcode == rv64::Opcode::multi_store_double) {
		gen::FulFill fulfill = fStoreReg(pInst->src1);
		gen::Add[I::I64::Const(pAddress + pInst->tempValue)];
		fulfill.now();
	}

	/* write the source value to the stack */
	fLoadFSrc2(pInst->opcode == rv64::Opcode::store_float || pInst->opcode == rv64::Opcode::multi_store_float);

	/* write the value to memory (memory-register maps 1-to-1 to memory-cache no matter if read or write) */
	switch (pInst->opcode) {
	case rv64::Opcode::store_float:
	case rv64::Opcode::multi_store_float:
		gen::Make->write(pInst->src1, gen::MemoryType::f32, pAddress, pNextAddress);
		break;
	case rv64::Opcode::store_double:
	case rv64::Opcode::multi_store_double:
		gen::Make->write(pInst->src1, gen::MemoryType::f64, pAddress, pNextAddress);
		break;
	default:
		break;
	}
}
void rv64::Translate::fMakeFloatToInt(bool iHalf, bool fHalf) {
	/* check if the frm is supported */
	if (pInst->misc != frm::roundNearestTiesToEven && pInst->misc != frm::dynamicRounding) {
		gen::Add[I::I64::Const(-pInst->misc)];
		gen::Make->invokeParam(pRegistered.frmFloatCsrWarn);
		gen::Add[I::Drop()];
	}

	/* check if the operation can be discarded */
	if (pInst->dest == reg::Zero)
		return;

	/* prepare the result writeback */
	gen::FulFill fulfill = fStoreDest();

	/* fetch the source operand */
	fLoadFSrc1(fHalf);

	/* either write the maximum value for nan to the stack, or write the actual value to the stack */
	bool isConvert = (pInst->opcode != rv64::Opcode::float_move_to_word && pInst->opcode != rv64::Opcode::double_move_to_dword);
	{
		wasm::IfThen _if;
		wasm::Variable temp;

		/* check if the value is nan (not necessary for moves) */
		if (isConvert) {
			temp = (fHalf ? fTempf32() : fTempf64());
			gen::Add[I::Local::Tee(temp)];
			gen::Add[I::Local::Get(temp)];
			gen::Add[fHalf ? I::F32::NotEqual() : I::F64::NotEqual()];
			_if = { gen::Sink, u8"", {}, { wasm::Type::i64 } };
		}

		/* write the maximum value out */
		switch (pInst->opcode) {
		case rv64::Opcode::float_convert_to_word_s:
		case rv64::Opcode::double_convert_to_word_s:
			gen::Add[I::U64::Const(std::numeric_limits<int32_t>::max())];
			break;
		case rv64::Opcode::float_convert_to_word_u:
		case rv64::Opcode::double_convert_to_word_u:
			gen::Add[I::U64::Const(std::numeric_limits<uint32_t>::max())];
			break;
		case rv64::Opcode::float_convert_to_dword_s:
		case rv64::Opcode::double_convert_to_dword_s:
			gen::Add[I::U64::Const(std::numeric_limits<int64_t>::max())];
			break;
		case rv64::Opcode::float_convert_to_dword_u:
		case rv64::Opcode::double_convert_to_dword_u:
			gen::Add[I::U64::Const(std::numeric_limits<uint64_t>::max())];
			break;
		case rv64::Opcode::float_move_to_word:
		case rv64::Opcode::double_move_to_dword:
		default:
			break;
		}

		/* recover the original value */
		if (isConvert) {
			_if.otherwise();
			gen::Add[I::Local::Get(temp)];
		}

		/* write the result of the operation to the stack */
		switch (pInst->opcode) {
		case rv64::Opcode::float_convert_to_word_s:
			gen::Add[I::I32::FromF32(false)];
			break;
		case rv64::Opcode::double_convert_to_word_s:
			gen::Add[I::I32::FromF64(false)];
			break;
		case rv64::Opcode::float_convert_to_word_u:
			gen::Add[I::U32::FromF32(false)];
			break;
		case rv64::Opcode::double_convert_to_word_u:
			gen::Add[I::U32::FromF64(false)];
			break;
		case rv64::Opcode::float_convert_to_dword_s:
			gen::Add[I::I64::FromF32(false)];
			break;
		case rv64::Opcode::double_convert_to_dword_s:
			gen::Add[I::I64::FromF64(false)];
			break;
		case rv64::Opcode::float_convert_to_dword_u:
			gen::Add[I::U64::FromF32(false)];
			break;
		case rv64::Opcode::double_convert_to_dword_u:
			gen::Add[I::U64::FromF64(false)];
			break;
		case rv64::Opcode::float_move_to_word:
			gen::Add[I::F32::AsInt()];
			break;
		case rv64::Opcode::double_move_to_dword:
			gen::Add[I::F64::AsInt()];
			break;
		default:
			break;
		}

		/* perform the sign extension to the 64-bit (also valid for moves, as ieee-754
		*	also has its sign bit in the highest position) and write the result back */
		if (iHalf)
			gen::Add[I::I32::Expand()];
	}
	fulfill.now();
}
void rv64::Translate::fMakeIntToFloat(bool iHalf, bool fHalf) const {
	/* check if the frm is supported */
	if (pInst->misc != frm::roundNearestTiesToEven && pInst->misc != frm::dynamicRounding) {
		gen::Add[I::I64::Const(-pInst->misc)];
		gen::Make->invokeParam(pRegistered.frmFloatCsrWarn);
		gen::Add[I::Drop()];
	}

	/* prepare the result writeback */
	gen::FulFill fulfill = fStoreFDest(fHalf);

	/* fetch the source operand */
	fLoadSrc1(true, iHalf);

	/* write the result of the operation to the stack */
	switch (pInst->opcode) {
	case rv64::Opcode::float_convert_from_word_s:
		gen::Add[I::I32::ToF32()];
		break;
	case rv64::Opcode::float_convert_from_word_u:
		gen::Add[I::U32::ToF32()];
		break;
	case rv64::Opcode::float_move_from_word:
		gen::Add[I::U32::AsFloat()];
		break;
	case rv64::Opcode::float_convert_from_dword_s:
		gen::Add[I::I64::ToF32()];
		break;
	case rv64::Opcode::float_convert_from_dword_u:
		gen::Add[I::U64::ToF32()];
		break;
	case rv64::Opcode::double_convert_from_word_s:
		gen::Add[I::I32::ToF64()];
		break;
	case rv64::Opcode::double_convert_from_word_u:
		gen::Add[I::U32::ToF64()];
		break;
	case rv64::Opcode::double_convert_from_dword_s:
		gen::Add[I::I64::ToF64()];
		break;
	case rv64::Opcode::double_convert_from_dword_u:
		gen::Add[I::U64::ToF64()];
		break;
	case rv64::Opcode::double_move_from_dword:
		gen::Add[I::U64::AsFloat()];
		break;
	default:
		break;
	}

	/* perform the float extension and write the result back */
	if (fHalf)
		fExpandFloat(false, true);
	fulfill.now();
}
void rv64::Translate::fMakeFloatALUSimple(bool half) const {
	/* check if the frm is supported */
	if (pInst->misc != frm::roundNearestTiesToEven && pInst->misc != frm::dynamicRounding) {
		gen::Add[I::I64::Const(-pInst->misc)];
		gen::Make->invokeParam(pRegistered.frmFloatCsrWarn);
		gen::Add[I::Drop()];
	}

	/* prepare the result writeback */
	gen::FulFill fulfill = fStoreFDest(half);

	/* fetch the source operands */
	fLoadFSrc1(half);
	fLoadFSrc2(half);

	/* write the result of the operation to the stack */
	switch (pInst->opcode) {
	case rv64::Opcode::float_add:
		gen::Add[I::F32::Add()];
		break;
	case rv64::Opcode::float_sub:
		gen::Add[I::F32::Sub()];
		break;
	case rv64::Opcode::float_mul:
		gen::Add[I::F32::Mul()];
		break;
	case rv64::Opcode::float_div:
		gen::Add[I::F32::Div()];
		break;
	case rv64::Opcode::float_min:
		gen::Add[I::F32::Min()];
		break;
	case rv64::Opcode::float_max:
		gen::Add[I::F32::Max()];
		break;
	case rv64::Opcode::double_add:
		gen::Add[I::F64::Add()];
		break;
	case rv64::Opcode::double_sub:
		gen::Add[I::F64::Sub()];
		break;
	case rv64::Opcode::double_mul:
		gen::Add[I::F64::Mul()];
		break;
	case rv64::Opcode::double_div:
		gen::Add[I::F64::Div()];
		break;
	case rv64::Opcode::double_min:
		gen::Add[I::F64::Min()];
		break;
	case rv64::Opcode::double_max:
		gen::Add[I::F64::Max()];
		break;
	default:
		break;
	}

	/* perform the float extension and write the result back */
	if (half)
		fExpandFloat(false, true);
	fulfill.now();
}
void rv64::Translate::fMakeFloatALULarge(bool half) const {
	/* check if the frm is supported */
	if (pInst->misc != frm::roundNearestTiesToEven && pInst->misc != frm::dynamicRounding) {
		gen::Add[I::I64::Const(-pInst->misc)];
		gen::Make->invokeParam(pRegistered.frmFloatCsrWarn);
		gen::Add[I::Drop()];
	}

	/* prepare the result writeback */
	gen::FulFill fulfill = fStoreFDest(half);

	/* fetch the first two source operands */

	/* fetch the source operands */
	fLoadFSrc1(half);
	fLoadFSrc2(half);

	/* perform the product operation */
	switch (pInst->opcode) {
	case rv64::Opcode::float_mul_add:
	case rv64::Opcode::float_mul_sub:
	case rv64::Opcode::float_neg_mul_add:
	case rv64::Opcode::float_neg_mul_sub:
		gen::Add[I::F32::Mul()];
		break;
	case rv64::Opcode::double_mul_add:
	case rv64::Opcode::double_mul_sub:
	case rv64::Opcode::double_neg_mul_add:
	case rv64::Opcode::double_neg_mul_sub:
		gen::Add[I::F64::Mul()];
		break;
	default:
		break;
	}

	/* fetch the third source operand */
	fLoadFSrc3(half);

	/* perform the addition or subtraction */
	switch (pInst->opcode) {
	case rv64::Opcode::float_mul_add:
	case rv64::Opcode::float_neg_mul_add:
		gen::Add[I::F32::Add()];
		break;
	case rv64::Opcode::float_mul_sub:
	case rv64::Opcode::float_neg_mul_sub:
		gen::Add[I::F32::Sub()];
		break;
	case rv64::Opcode::double_mul_add:
	case rv64::Opcode::double_neg_mul_add:
		gen::Add[I::F64::Add()];
		break;
	case rv64::Opcode::double_mul_sub:
	case rv64::Opcode::double_neg_mul_sub:
		gen::Add[I::F64::Sub()];
		break;
	default:
		break;
	}

	/* perform the inversion */
	switch (pInst->opcode) {
	case rv64::Opcode::float_neg_mul_add:
	case rv64::Opcode::float_neg_mul_sub:
		gen::Add[I::F32::Negate()];
		break;
	case rv64::Opcode::double_neg_mul_add:
	case rv64::Opcode::double_neg_mul_sub:
		gen::Add[I::F64::Negate()];
		break;
	default:
		break;
	}

	/* perform the float extension and write the result back */
	if (half)
		fExpandFloat(false, true);
	fulfill.now();
}
void rv64::Translate::fMakeFloatConvert() const {
	/* check if the frm is supported */
	if (pInst->misc != frm::roundNearestTiesToEven && pInst->misc != frm::dynamicRounding) {
		gen::Add[I::I64::Const(-pInst->misc)];
		gen::Make->invokeParam(pRegistered.frmFloatCsrWarn);
		gen::Add[I::Drop()];
	}

	/* prepare the result writeback */
	gen::FulFill fulfill = fStoreFDest(pInst->opcode == rv64::Opcode::double_to_float);

	/* write the source operand */
	fLoadFSrc1(pInst->opcode == rv64::Opcode::float_to_double);

	/* perform the operation */
	if (pInst->opcode == rv64::Opcode::float_to_double)
		gen::Add[I::F32::Expand()];
	else
		gen::Add[I::F64::Shrink()];

	/* perform the float extension and write the result back */
	if (pInst->opcode == rv64::Opcode::double_to_float)
		fExpandFloat(false, true);
	fulfill.now();
}
void rv64::Translate::fMakeFloatSign(bool half) const {
	/* check if the frm is supported */
	if (pInst->misc != frm::roundNearestTiesToEven && pInst->misc != frm::dynamicRounding) {
		gen::Add[I::I64::Const(-pInst->misc)];
		gen::Make->invokeParam(pRegistered.frmFloatCsrWarn);
		gen::Add[I::Drop()];
	}

	/* check if the result will already be an integer */
	bool toInt = (pInst->opcode == rv64::Opcode::float_sign_xor || pInst->opcode == rv64::Opcode::double_sign_xor);

	/* prepare the result writeback */
	gen::FulFill fulfill = fStoreFDest(half || toInt);

	/* fetch the source operands and check if the first operand needs to be converted to an integer */
	fLoadFSrc1(half);
	if (toInt)
		gen::Add[half ? I::F32::AsInt() : I::F64::AsInt()];
	fLoadFSrc2(half);

	/* write the result of the operation to the stack */
	switch (pInst->opcode) {
	case rv64::Opcode::float_sign_copy:
		gen::Add[I::F32::CopySign()];
		break;
	case rv64::Opcode::float_sign_invert:
		gen::Add[I::F32::Negate()];
		gen::Add[I::F32::CopySign()];
		break;
	case rv64::Opcode::float_sign_xor:
		/* extract the sign of the second operand and xor it with the first operand */
		gen::Add[I::F32::AsInt()];
		gen::Add[I::U32::Const(uint64_t(1) << 31)];
		gen::Add[I::U32::And()];
		gen::Add[I::U32::XOr()];
		break;
	case rv64::Opcode::double_sign_copy:
		gen::Add[I::F64::CopySign()];
		break;
	case rv64::Opcode::double_sign_invert:
		gen::Add[I::F64::Negate()];
		gen::Add[I::F64::CopySign()];
		break;
	case rv64::Opcode::double_sign_xor:
		/* extract the sign of the second operand and xor it with the first operand */
		gen::Add[I::F64::AsInt()];
		gen::Add[I::U64::Const(uint64_t(1) << 63)];
		gen::Add[I::U64::And()];
		gen::Add[I::U64::XOr()];
		break;
	default:
		break;
	}

	/* perform the float extension and write the result back */
	if (half)
		fExpandFloat(toInt, true);
	fulfill.now();
}
void rv64::Translate::fMakeFloatCompare(bool half) const {
	/* check if the frm is supported */
	if (pInst->misc != frm::roundNearestTiesToEven && pInst->misc != frm::dynamicRounding) {
		gen::Add[I::I64::Const(-pInst->misc)];
		gen::Make->invokeParam(pRegistered.frmFloatCsrWarn);
		gen::Add[I::Drop()];
	}

	/* prepare the result writeback */
	gen::FulFill fulfill = fStoreDest();

	/* fetch the source operands */
	fLoadFSrc1(half);
	fLoadFSrc2(half);

	/* write the result of the operation to the stack */
	switch (pInst->opcode) {
	case rv64::Opcode::float_equal:
		gen::Add[I::F32::Equal()];
		break;
	case rv64::Opcode::float_less_equal:
		gen::Add[I::F32::LessEqual()];
		break;
	case rv64::Opcode::float_less_than:
		gen::Add[I::F32::Less()];
		break;
	case rv64::Opcode::double_equal:
		gen::Add[I::F64::Equal()];
		break;
	case rv64::Opcode::double_less_equal:
		gen::Add[I::F64::LessEqual()];
		break;
	case rv64::Opcode::double_less_than:
		gen::Add[I::F64::Less()];
		break;
	default:
		break;
	}

	/* perform the result extension and write the result back */
	gen::Add[I::U32::Expand()];
	fulfill.now();
}
void rv64::Translate::fMakeFloatUnary(bool half, bool intResult) const {
	/* check if the frm is supported */
	if (pInst->misc != frm::roundNearestTiesToEven && pInst->misc != frm::dynamicRounding) {
		gen::Add[I::I64::Const(-pInst->misc)];
		gen::Make->invokeParam(pRegistered.frmFloatCsrWarn);
		gen::Add[I::Drop()];
	}

	/* prepare the result writeback */
	gen::FulFill fulfill = (intResult ? fStoreDest() : fStoreFDest(half));

	/* fetch the source operand */
	fLoadFSrc1(half);

	/* write the result of the operation to the stack */
	switch (pInst->opcode) {
	case rv64::Opcode::float_sqrt:
		gen::Add[I::F32::SquareRoot()];
		break;
	case rv64::Opcode::double_sqrt:
		gen::Add[I::F64::SquareRoot()];
		break;
	case rv64::Opcode::float_classify:
		gen::Add[I::F32::AsInt()];
		gen::Add[I::U32::Expand()];
		gen::Make->invokeParam(pRegistered.classify32Bit);
		break;
	case rv64::Opcode::double_classify:
		gen::Add[I::F64::AsInt()];
		gen::Make->invokeParam(pRegistered.classify64Bit);
		break;
	default:
		break;
	}

	/* perform the float extension and write the result back */
	if (half && !intResult)
		fExpandFloat(false, true);
	fulfill.now();
}

bool rv64::Translate::setup() {
	/* register the callbacks */
	pRegistered.classify32Bit = env::Instance()->interact().defineCallback([](uint64_t value) -> uint64_t {
		return fClassifyf32(std::bit_cast<float, uint32_t>(uint32_t(value)));
		});
	pRegistered.classify64Bit = env::Instance()->interact().defineCallback([](uint64_t value) -> uint64_t {
		return fClassifyf64(std::bit_cast<double, uint64_t>(value));
		});
	pRegistered.readFloatCsrWarn = env::Instance()->interact().defineCallback([this]() {
		if (!pReadCsrShown)
			logger.warn(u8"Reading csr::float");
		pReadCsrShown = true;
		});
	pRegistered.frmFloatCsrWarn = env::Instance()->interact().defineCallback([this](int64_t value) -> uint64_t {
		if (pFrmFloatShown)
			return 0;
		pFrmFloatShown = true;
		if (value < 0)
			logger.warn(u8"Unsupported frm used in float instruction [", str::As{ U"03b", -value }, u8']');
		else
			logger.warn(u8"Setting csr::float::frm to unsupported [", str::As{ U"03b", value }, u8']');
		return 0;
		});
	return true;
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
		pWriter->makeException(Translate::MisalignedException, pAddress, pNextAddress);
		break;
	case rv64::Opcode::illegal_instruction:
		pWriter->makeException(Translate::IllegalException, pAddress, pNextAddress);
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
		fMakeCSR();
		break;
	case rv64::Opcode::float_convert_to_word_s:
	case rv64::Opcode::float_convert_to_word_u:
	case rv64::Opcode::float_move_to_word:
		fMakeFloatToInt(true, true);
		break;
	case rv64::Opcode::float_convert_to_dword_s:
	case rv64::Opcode::float_convert_to_dword_u:
		fMakeFloatToInt(false, true);
		break;
	case rv64::Opcode::double_convert_to_word_s:
	case rv64::Opcode::double_convert_to_word_u:
		fMakeFloatToInt(true, false);
		break;
	case rv64::Opcode::double_convert_to_dword_s:
	case rv64::Opcode::double_convert_to_dword_u:
	case rv64::Opcode::double_move_to_dword:
		fMakeFloatToInt(false, false);
		break;
	case rv64::Opcode::float_convert_from_word_s:
	case rv64::Opcode::float_convert_from_word_u:
	case rv64::Opcode::float_move_from_word:
		fMakeIntToFloat(true, true);
		break;
	case rv64::Opcode::float_convert_from_dword_s:
	case rv64::Opcode::float_convert_from_dword_u:
		fMakeIntToFloat(false, true);
		break;
	case rv64::Opcode::double_convert_from_word_s:
	case rv64::Opcode::double_convert_from_word_u:
		fMakeIntToFloat(true, false);
		break;
	case rv64::Opcode::double_convert_from_dword_s:
	case rv64::Opcode::double_convert_from_dword_u:
	case rv64::Opcode::double_move_from_dword:
		fMakeIntToFloat(false, false);
		break;
	case rv64::Opcode::float_add:
	case rv64::Opcode::float_sub:
	case rv64::Opcode::float_mul:
	case rv64::Opcode::float_div:
	case rv64::Opcode::float_min:
	case rv64::Opcode::float_max:
		fMakeFloatALUSimple(true);
		break;
	case rv64::Opcode::double_add:
	case rv64::Opcode::double_sub:
	case rv64::Opcode::double_mul:
	case rv64::Opcode::double_div:
	case rv64::Opcode::double_min:
	case rv64::Opcode::double_max:
		fMakeFloatALUSimple(false);
		break;
	case rv64::Opcode::float_mul_add:
	case rv64::Opcode::float_mul_sub:
	case rv64::Opcode::float_neg_mul_add:
	case rv64::Opcode::float_neg_mul_sub:
		fMakeFloatALULarge(true);
		break;
	case rv64::Opcode::double_mul_add:
	case rv64::Opcode::double_mul_sub:
	case rv64::Opcode::double_neg_mul_add:
	case rv64::Opcode::double_neg_mul_sub:
		fMakeFloatALULarge(false);
		break;
	case rv64::Opcode::float_to_double:
	case rv64::Opcode::double_to_float:
		fMakeFloatConvert();
		break;
	case rv64::Opcode::float_sign_copy:
	case rv64::Opcode::float_sign_invert:
	case rv64::Opcode::float_sign_xor:
		fMakeFloatSign(true);
		break;
	case rv64::Opcode::double_sign_copy:
	case rv64::Opcode::double_sign_invert:
	case rv64::Opcode::double_sign_xor:
		fMakeFloatSign(false);
		break;
	case rv64::Opcode::float_equal:
	case rv64::Opcode::float_less_equal:
	case rv64::Opcode::float_less_than:
		fMakeFloatCompare(true);
		break;
	case rv64::Opcode::double_equal:
	case rv64::Opcode::double_less_equal:
	case rv64::Opcode::double_less_than:
		fMakeFloatCompare(false);
		break;
	case rv64::Opcode::float_sqrt:
		fMakeFloatUnary(true, false);
		break;
	case rv64::Opcode::double_sqrt:
		fMakeFloatUnary(false, false);
		break;
	case rv64::Opcode::float_classify:
		fMakeFloatUnary(true, true);
		break;
	case rv64::Opcode::double_classify:
		fMakeFloatUnary(false, true);
		break;

	case rv64::Opcode::_invalid:
		/* raise the not-implemented exception for all remaining instructions */
		pWriter->makeException(Translate::NotImplException, pAddress, pNextAddress);
		break;
	}
}
