#pragma once

#include "rv64-common.h"

namespace rv64 {
	enum class Opcode : uint16_t {
		load_upper_imm,
		add_upper_imm_pc,
		jump_and_link_imm,
		jump_and_link_reg,
		branch_eq,
		branch_ne,
		branch_lt_s,
		branch_ge_s,
		branch_lt_u,
		branch_ge_u,
		add_imm,
		set_less_than_s_imm,
		set_less_than_u_imm,
		xor_imm,
		or_imm,
		and_imm,
		shift_left_logic_imm,
		shift_right_logic_imm,
		shift_right_arith_imm,
		add_reg,
		sub_reg,
		mul_reg,
		mul_high_s_reg,
		mul_high_s_u_reg,
		mul_high_u_reg,
		div_s_reg,
		div_u_reg,
		rem_s_reg,
		rem_u_reg,
		set_less_than_s_reg,
		set_less_than_u_reg,
		xor_reg,
		or_reg,
		and_reg,
		shift_left_logic_reg,
		shift_right_logic_reg,
		shift_right_arith_reg,
		no_op,
		move,
		_invalid
	};
	enum class Format : uint8_t {
		none,
		dst_src1,
		dst_src1_imm,
		dst_src1_src2,
		dst_imm,
		src1_src2
	};

	struct Instruction {
		rv64::Opcode opcode = rv64::Opcode::_invalid;
		rv64::Format format = Format::none;
		uint8_t regDest = 0;
		uint8_t regSrc1 = 0;
		uint8_t regSrc2 = 0;
		int64_t immediate = 0;
	};

	rv64::Instruction Decode(uint32_t data);
}
