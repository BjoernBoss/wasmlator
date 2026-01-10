/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025-2026 Bjoern Boss Henrichsen */
#include "rv64-pseudo.h"

rv64::DetectPseudo::DetectPseudo(bool multi) : pState{ multi ? State::multi : State::none } {}

void rv64::DetectPseudo::fRestore(rv64::Instruction& inst) const {
	if (pState == State::auipc)
		inst.opcode = rv64::Opcode::add_upper_imm_pc;
	else
		inst.opcode = rv64::Opcode::load_upper_imm;
	inst.imm = pOriginal.imm;
	inst.dest = pOriginal.reg;
	inst.size = pOriginal.size;
}
void rv64::DetectPseudo::fCloseLI(rv64::Instruction& inst) const {
	inst.opcode = rv64::Opcode::multi_load_imm;
	inst.imm = pLUI.imm;
	inst.size = pLUI.size;
	inst.dest = pOriginal.reg;
}
bool rv64::DetectPseudo::fMatchFirst(rv64::Instruction& inst) {
	switch (inst.opcode) {
	case rv64::Opcode::add_imm:
		/* li */
		if (inst.src1 == reg::Zero)
			inst.opcode = rv64::Opcode::multi_load_imm;

		/* mv */
		else if (inst.imm == 0)
			inst.pseudo = rv64::Pseudo::mv;
		break;
	case rv64::Opcode::xor_imm:
		/* not */
		if (inst.imm == -1)
			inst.pseudo = rv64::Pseudo::inv;
		break;
	case rv64::Opcode::sub_reg:
		/* neg */
		if (inst.src1 == reg::Zero)
			inst.pseudo = rv64::Pseudo::neg;
		break;
	case rv64::Opcode::sub_reg_half:
		/* negw */
		if (inst.src1 == reg::Zero)
			inst.pseudo = rv64::Pseudo::neg_half;
		break;
	case rv64::Opcode::add_imm_half:
		/* sext.w */
		if (inst.imm == 0)
			inst.pseudo = rv64::Pseudo::sign_extend;
		break;
	case rv64::Opcode::set_less_than_u_imm:
		/* seqz */
		if (inst.imm == 1)
			inst.pseudo = rv64::Pseudo::set_eq_zero;
		break;
	case rv64::Opcode::set_less_than_u_reg:
		/* snez */
		if (inst.src1 == reg::Zero)
			inst.pseudo = rv64::Pseudo::set_ne_zero;
		break;
	case rv64::Opcode::set_less_than_s_reg:
		/* sltz */
		if (inst.src2 == reg::Zero)
			inst.pseudo = rv64::Pseudo::set_lt_zero;

		/* sgtz */
		else if (inst.src1 == reg::Zero)
			inst.pseudo = rv64::Pseudo::set_gt_zero;
		break;
	case rv64::Opcode::float_sign_copy:
		/* fmv.s */
		if (inst.src1 == inst.src2)
			inst.pseudo = rv64::Pseudo::float_copy_sign;
		break;
	case rv64::Opcode::float_sign_xor:
		/* fabs.s */
		if (inst.src1 == inst.src2)
			inst.pseudo = rv64::Pseudo::float_abs;
		break;
	case rv64::Opcode::float_sign_invert:
		/* fneg.s */
		if (inst.src1 == inst.src2)
			inst.pseudo = rv64::Pseudo::float_neg;
		break;
	case rv64::Opcode::double_sign_copy:
		/* fmv.s */
		if (inst.src1 == inst.src2)
			inst.pseudo = rv64::Pseudo::double_copy_sign;
		break;
	case rv64::Opcode::double_sign_xor:
		/* fabs.s */
		if (inst.src1 == inst.src2)
			inst.pseudo = rv64::Pseudo::double_abs;
		break;
	case rv64::Opcode::double_sign_invert:
		/* fneg.s */
		if (inst.src1 == inst.src2)
			inst.pseudo = rv64::Pseudo::double_neg;
		break;
	case rv64::Opcode::branch_eq:
		/* beqz */
		if (inst.src2 == reg::Zero)
			inst.pseudo = rv64::Pseudo::branch_eqz;
		break;
	case rv64::Opcode::branch_ne:
		/* bnez */
		if (inst.src2 == reg::Zero)
			inst.pseudo = rv64::Pseudo::branch_nez;
		break;
	case rv64::Opcode::branch_ge_s:
		/* blez */
		if (inst.src1 == reg::Zero)
			inst.pseudo = rv64::Pseudo::branch_lez;

		/* bgez */
		else if (inst.src2 == reg::Zero)
			inst.pseudo = rv64::Pseudo::branch_gez;
		break;
	case rv64::Opcode::branch_lt_s:
		/* bltz */
		if (inst.src2 == reg::Zero)
			inst.pseudo = rv64::Pseudo::branch_ltz;

		/* bgtz */
		else if (inst.src1 == reg::Zero)
			inst.pseudo = rv64::Pseudo::branch_gtz;
		break;
	case rv64::Opcode::jump_and_link_imm:
		/* j */
		if (inst.dest == reg::Zero)
			inst.pseudo = rv64::Pseudo::jump;

		/* jal */
		else if (inst.dest == reg::X1)
			inst.pseudo = rv64::Pseudo::jump_and_link;
		break;
	case rv64::Opcode::jump_and_link_reg:
		/* ret */
		if (inst.dest == reg::Zero && inst.src1 == reg::X1 && inst.imm == 0)
			inst.pseudo = rv64::Pseudo::ret;

		/* jr */
		else if (inst.dest == reg::Zero && inst.imm == 0)
			inst.pseudo = rv64::Pseudo::jump_reg;

		/* jalr */
		else if (inst.dest == reg::X1 && inst.imm == 0)
			inst.pseudo = rv64::Pseudo::jump_and_link_reg;
		break;
	case rv64::Opcode::fence:
		/* fence */
		if (inst.misc == 0xff)
			inst.pseudo = rv64::Pseudo::fence;
		break;
	case rv64::Opcode::csr_read_and_set:
		/* csrr */
		if (inst.src1 == reg::Zero)
			inst.pseudo = rv64::Pseudo::csr_read;

		/* csrs */
		else if (inst.dest == reg::Zero)
			inst.pseudo = rv64::Pseudo::csr_set_bits;

		/* frcsr */
		else if (inst.src1 == reg::Zero && inst.misc == csr::fpStatus)
			inst.pseudo = rv64::Pseudo::csr_float_read_status;

		/* frrm */
		else if (inst.src1 == reg::Zero && inst.misc == csr::fpRoundingMode)
			inst.pseudo = rv64::Pseudo::csr_float_read_rm;

		/* frflags */
		else if (inst.src1 == reg::Zero && inst.misc == csr::fpExceptionFlags)
			inst.pseudo = rv64::Pseudo::csr_float_read_exceptions;

		/* rdinstret */
		else if (inst.src1 == reg::Zero && inst.misc == csr::instRetired)
			inst.pseudo = rv64::Pseudo::csr_read_inst_retired;

		/* rdcycle */
		else if (inst.src1 == reg::Zero && inst.misc == csr::cycles)
			inst.pseudo = rv64::Pseudo::csr_read_cycles;

		/* rdtime */
		else if (inst.src1 == reg::Zero && inst.misc == csr::realTime)
			inst.pseudo = rv64::Pseudo::csr_read_time;
		break;
	case rv64::Opcode::csr_read_write:
		/* csrw */
		if (inst.dest == reg::Zero)
			inst.pseudo = rv64::Pseudo::csr_write;

		/* fscsr */
		else if (inst.misc == csr::fpStatus) {
			if (inst.dest == reg::Zero)
				inst.pseudo = rv64::Pseudo::csr_float_write_status;
			else
				inst.pseudo = rv64::Pseudo::csr_float_swap_status;
		}

		/* fsrm */
		else if (inst.misc == csr::fpRoundingMode) {
			if (inst.dest == reg::Zero)
				inst.pseudo = rv64::Pseudo::csr_float_write_rm;
			else
				inst.pseudo = rv64::Pseudo::csr_float_swap_rm;
		}

		/* fsflags */
		else if (inst.misc == csr::fpExceptionFlags) {
			if (inst.dest == reg::Zero)
				inst.pseudo = rv64::Pseudo::csr_float_write_exceptions;
			else
				inst.pseudo = rv64::Pseudo::csr_float_swap_exceptions;
		}
		break;
	case rv64::Opcode::csr_read_and_clear:
		/* csrc */
		if (inst.dest == reg::Zero)
			inst.pseudo = rv64::Pseudo::csr_clear_bits;
		break;
	case rv64::Opcode::csr_read_and_set_imm:
		/* csrsi */
		if (inst.dest == reg::Zero)
			inst.pseudo = rv64::Pseudo::csr_set_bits_imm;
		break;
	case rv64::Opcode::csr_read_write_imm:
		/* csrwi */
		if (inst.dest == reg::Zero)
			inst.pseudo = rv64::Pseudo::csr_write_imm;

		/* fsrmi */
		else if (inst.misc == csr::fpRoundingMode) {
			if (inst.dest == reg::Zero)
				inst.pseudo = rv64::Pseudo::csr_float_write_rm_imm;
			else
				inst.pseudo = rv64::Pseudo::csr_float_swap_rm_imm;
		}

		/* fsflagsi */
		else if (inst.misc == csr::fpExceptionFlags) {
			if (inst.dest == reg::Zero)
				inst.pseudo = rv64::Pseudo::csr_float_write_exceptions_imm;
			else
				inst.pseudo = rv64::Pseudo::csr_float_swap_exceptions_imm;
		}
		break;
	case rv64::Opcode::csr_read_and_clear_imm:
		/* csrci */
		if (inst.dest == reg::Zero)
			inst.pseudo = rv64::Pseudo::csr_clear_bits_imm;
		break;
	case rv64::Opcode::add_upper_imm_pc:
		if (inst.dest != reg::Zero) {
			pState = State::auipc;
			return true;
		}
		break;
	case rv64::Opcode::load_upper_imm:
		pState = State::lui;
		pLUI.size = inst.size;
		pLUI.imm = inst.imm;
		return true;
	default:
		break;
	}
	return false;
}
rv64::DetectPseudo::Result rv64::DetectPseudo::fContinueAUIPC(rv64::Instruction& inst) const {
	switch (inst.opcode) {
	case rv64::Opcode::add_imm:
		/* la */
		if (inst.dest != pOriginal.reg || inst.src1 != pOriginal.reg)
			return Result::restore;
		inst.opcode = rv64::Opcode::multi_load_address;
		break;
	case rv64::Opcode::load_byte_s:
		/* lb */
		if (inst.dest != pOriginal.reg || inst.src1 != pOriginal.reg)
			return Result::restore;
		inst.opcode = rv64::Opcode::multi_load_byte_s;
		break;
	case rv64::Opcode::load_byte_u:
		/* lbu */
		if (inst.dest != pOriginal.reg || inst.src1 != pOriginal.reg)
			return Result::restore;
		inst.opcode = rv64::Opcode::multi_load_byte_u;
		break;
	case rv64::Opcode::load_half_s:
		/* lh */
		if (inst.dest != pOriginal.reg || inst.src1 != pOriginal.reg)
			return Result::restore;
		inst.opcode = rv64::Opcode::multi_load_half_s;
		break;
	case rv64::Opcode::load_half_u:
		/* lhu */
		if (inst.dest != pOriginal.reg || inst.src1 != pOriginal.reg)
			return Result::restore;
		inst.opcode = rv64::Opcode::multi_load_half_u;
		break;
	case rv64::Opcode::load_word_s:
		/* lw */
		if (inst.dest != pOriginal.reg || inst.src1 != pOriginal.reg)
			return Result::restore;
		inst.opcode = rv64::Opcode::multi_load_word_s;
		break;
	case rv64::Opcode::load_word_u:
		/* lwu */
		if (inst.dest != pOriginal.reg || inst.src1 != pOriginal.reg)
			return Result::restore;
		inst.opcode = rv64::Opcode::multi_load_word_u;
		break;
	case rv64::Opcode::load_dword:
		/* ld */
		if (inst.dest != pOriginal.reg || inst.src1 != pOriginal.reg)
			return Result::restore;
		inst.opcode = rv64::Opcode::multi_load_dword;
		break;
	case rv64::Opcode::load_float:
		/* flw */
		if (inst.src1 != pOriginal.reg)
			return Result::restore;
		inst.opcode = rv64::Opcode::multi_load_float;
		break;
	case rv64::Opcode::load_double:
		/* fld */
		if (inst.src1 != pOriginal.reg)
			return Result::restore;
		inst.opcode = rv64::Opcode::multi_load_double;
		break;
	case rv64::Opcode::store_byte:
		/* sb */
		if (inst.src1 != pOriginal.reg)
			return Result::restore;
		inst.opcode = rv64::Opcode::multi_store_byte;
		break;
	case rv64::Opcode::store_half:
		/* sh */
		if (inst.src1 != pOriginal.reg)
			return Result::restore;
		inst.opcode = rv64::Opcode::multi_store_half;
		break;
	case rv64::Opcode::store_word:
		/* sw */
		if (inst.src1 != pOriginal.reg)
			return Result::restore;
		inst.opcode = rv64::Opcode::multi_store_word;
		break;
	case rv64::Opcode::store_dword:
		/* sd */
		if (inst.src1 != pOriginal.reg)
			return Result::restore;
		inst.opcode = rv64::Opcode::multi_store_dword;
		break;
	case rv64::Opcode::store_float:
		/* fsw */
		if (inst.src1 != pOriginal.reg)
			return Result::restore;
		inst.opcode = rv64::Opcode::multi_store_float;
		break;
	case rv64::Opcode::store_double:
		/* fsd */
		if (inst.src1 != pOriginal.reg)
			return Result::restore;
		inst.opcode = rv64::Opcode::multi_store_double;
		break;
	case rv64::Opcode::jump_and_link_reg:
		/* call */
		if (pOriginal.reg == reg::X6 && inst.src1 == reg::X6 && inst.dest == reg::X1)
			inst.opcode = rv64::Opcode::multi_call;

		/* tail */
		else if (pOriginal.reg == reg::X6 && inst.src1 == reg::X6 && inst.dest == reg::Zero)
			inst.opcode = rv64::Opcode::multi_tail;
		else
			return Result::restore;
		break;
	default:
		break;
	}

	/* setup the completed instruction */
	inst.tempValue = pOriginal.imm;
	inst.imm = pOriginal.imm + inst.imm;
	inst.size = pOriginal.size + inst.size;
	return Result::success;
}
rv64::DetectPseudo::Result rv64::DetectPseudo::fContinueLUI(rv64::Instruction& inst) {
	/* check if this is a continuation for the load-immediate  */
	if (inst.src1 == pOriginal.reg && inst.dest == pOriginal.reg) {
		/* check if another shift can be added */
		if (inst.opcode == rv64::Opcode::shift_left_logic_imm && pState == State::luiAdded) {
			pLUI.imm <<= inst.imm;
			pLUI.size += inst.size;
			pState = State::luiShifted;
			return Result::incomplete;
		}

		/* check if another addition can be added */
		if (inst.opcode == rv64::Opcode::add_imm && (pState == State::lui || pState == State::luiShifted)) {
			pLUI.imm += inst.imm;
			pLUI.size += inst.size;
			pState = State::luiAdded;
			return Result::incomplete;
		}
	}

	/* check if a valid immediate-sequence has been foudn */
	if (pState == State::lui)
		return Result::restore;

	/* setup the complete instruction */
	fCloseLI(inst);
	return Result::success;
}

bool rv64::DetectPseudo::next(rv64::Instruction& inst) {
	/* check if this is the first instruction and try to match it */
	if (pState == State::none || pState == State::multi) {
		/* check if another instruction needs to be added */
		if (!fMatchFirst(inst))
			return false;
		if (pState != State::multi)
			return false;

		/* cache the original instruction */
		pOriginal = { inst.imm, inst.dest, inst.size };
		return true;
	}

	/* handle the remaining states */
	Result result = Result::restore;
	if (pState == State::auipc)
		result = fContinueAUIPC(inst);
	else if (pState == State::lui || pState == State::luiShifted || pState == State::luiAdded)
		result = fContinueLUI(inst);

	/* check if the state should be restored and if the pseudo-checker is done */
	if (result == Result::restore)
		fRestore(inst);
	return (result == Result::incomplete);
}
rv64::Instruction rv64::DetectPseudo::close() const {
	rv64::Instruction out;

	/* check if the state simply needs to be restored */
	if (pState == State::auipc || pState == State::lui)
		fRestore(out);
	else
		fCloseLI(out);
	return out;
}
