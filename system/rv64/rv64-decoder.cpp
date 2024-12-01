#include "rv64-decoder.h"

rv64::Instruction rv64::detail::Opcode13(uint32_t data) {
	rv64::Instruction out;

	out.format = rv64::Format::dst_src1_imm;
	out.dest = detail::GetU<7, 11>(data);
	out.src1 = detail::GetU<15, 19>(data);
	out.imm = detail::GetS<20, 31>(data);

	/* RV64I */
	uint32_t funct6 = detail::GetU<26, 31>(data);

	switch (detail::GetU<12, 14>(data)) {
	case 0x00:
		out.opcode = rv64::Opcode::add_imm;

		/* special-case */
		if (out.imm == 0) {
			if (out.dest == 0 && out.src1 == 0) {
				out.opcode = rv64::Opcode::no_op;
				out.format = rv64::Format::none;
			}
			else {
				out.opcode = rv64::Opcode::move;
				out.format = rv64::Format::dst_src1;
			}
		}
		break;
	case 0x01:
		if (funct6 == 0x00) {
			out.opcode = rv64::Opcode::shift_left_logic_imm;
			out.imm &= 0x3f;
		}
		break;
	case 0x02:
		out.opcode = rv64::Opcode::set_less_than_s_imm;
		break;
	case 0x03:
		out.opcode = rv64::Opcode::set_less_than_u_imm;
		break;
	case 0x04:
		out.opcode = rv64::Opcode::xor_imm;
		break;
	case 0x05:
		if (funct6 == 0x00) {
			out.opcode = rv64::Opcode::shift_right_logic_imm;
			out.imm &= 0x3f;
		}
		else if (funct6 == 0x10) {
			out.opcode = rv64::Opcode::shift_right_arith_imm;
			out.imm &= 0x3f;
		}
		break;
	case 0x06:
		out.opcode = rv64::Opcode::or_imm;
		break;
	case 0x07:
		out.opcode = rv64::Opcode::and_imm;
		break;
	}

	return out;
}
rv64::Instruction rv64::detail::Opcode17(uint32_t data) {
	rv64::Instruction out;

	out.format = rv64::Format::dst_imm;
	out.dest = detail::GetU<7, 11>(data);
	out.imm = int32_t(data & ~0x0fff);

	out.opcode = rv64::Opcode::add_upper_imm_pc;

	return out;
}
rv64::Instruction rv64::detail::Opcode33(uint32_t data) {
	rv64::Instruction out;

	out.format = rv64::Format::dst_src1_src2;
	out.dest = detail::GetU<7, 11>(data);
	out.src1 = detail::GetU<15, 19>(data);
	out.src2 = detail::GetU<20, 24>(data);

	uint32_t funct7 = detail::GetU<25, 31>(data);

	switch (detail::GetU<12, 14>(data)) {
	case 0x00:
		if (funct7 == 0x00)
			out.opcode = rv64::Opcode::add_reg;
		else if (funct7 == 0x20)
			out.opcode = rv64::Opcode::sub_reg;
		else if (funct7 == 0x01)
			out.opcode = rv64::Opcode::mul_reg;
		break;
	case 0x01:
		if (funct7 == 0x00)
			out.opcode = rv64::Opcode::shift_left_logic_reg;
		else if (funct7 == 0x01)
			out.opcode = rv64::Opcode::mul_high_s_reg;
		break;
	case 0x02:
		if (funct7 == 0x00)
			out.opcode = rv64::Opcode::set_less_than_s_reg;
		else if (funct7 == 0x01)
			out.opcode = rv64::Opcode::mul_high_s_u_reg;
		break;
	case 0x03:
		if (funct7 == 0x00)
			out.opcode = rv64::Opcode::set_less_than_u_reg;
		else if (funct7 == 0x01)
			out.opcode = rv64::Opcode::mul_high_u_reg;
		break;
	case 0x04:
		if (funct7 == 0x00)
			out.opcode = rv64::Opcode::xor_reg;
		else if (funct7 == 0x01)
			out.opcode = rv64::Opcode::div_s_reg;
		break;
	case 0x05:
		if (funct7 == 0x00)
			out.opcode = rv64::Opcode::shift_right_logic_reg;
		else if (funct7 == 0x20)
			out.opcode = rv64::Opcode::shift_right_arith_reg;
		else if (funct7 == 0x01)
			out.opcode = rv64::Opcode::div_u_reg;
		break;
	case 0x06:
		if (funct7 == 0x00)
			out.opcode = rv64::Opcode::or_reg;
		else if (funct7 == 0x01)
			out.opcode = rv64::Opcode::rem_s_reg;
		break;
	case 0x07:
		if (funct7 == 0x00)
			out.opcode = rv64::Opcode::and_reg;
		else if (funct7 == 0x01)
			out.opcode = rv64::Opcode::rem_u_reg;
		break;
	}

	return out;
}
rv64::Instruction rv64::detail::Opcode37(uint32_t data) {
	rv64::Instruction out;

	out.format = rv64::Format::dst_imm;
	out.dest = detail::GetU<7, 11>(data);
	out.imm = int32_t(data & ~0x0fff);

	out.opcode = rv64::Opcode::load_upper_imm;

	return out;
}
rv64::Instruction rv64::detail::Opcode63(uint32_t data) {
	rv64::Instruction out;

	out.format = rv64::Format::src1_src2;
	out.src1 = detail::GetU<15, 19>(data);
	out.src2 = detail::GetU<20, 24>(data);
	out.imm = (int64_t(detail::GetS<31, 31>(data)) << 12)
		| (uint64_t(detail::GetU<7, 7>(data)) << 11)
		| (uint64_t(detail::GetU<25, 30>(data)) << 5)
		| (uint64_t(detail::GetU<8, 11>(data)) << 1);

	switch (detail::GetU<12, 14>(data)) {
	case 0x00:
		out.opcode = rv64::Opcode::branch_eq;
		break;
	case 0x01:
		out.opcode = rv64::Opcode::branch_ne;
		break;
	case 0x04:
		out.opcode = rv64::Opcode::branch_lt_s;
		break;
	case 0x05:
		out.opcode = rv64::Opcode::branch_ge_s;
		break;
	case 0x06:
		out.opcode = rv64::Opcode::branch_lt_u;
		break;
	case 0x07:
		out.opcode = rv64::Opcode::branch_ge_u;
		break;
	}

	return out;
}
rv64::Instruction rv64::detail::Opcode67(uint32_t data) {
	rv64::Instruction out;

	out.format = rv64::Format::dst_src1_imm;
	out.dest = detail::GetU<7, 11>(data);
	out.src1 = detail::GetU<15, 19>(data);
	out.imm = detail::GetS<20, 31>(data);

	if (detail::GetU<12, 14>(data) == 0x00)
		out.opcode = rv64::Opcode::jump_and_link_reg;

	return out;
}
rv64::Instruction rv64::detail::Opcode6f(uint32_t data) {
	rv64::Instruction out;

	out.format = rv64::Format::dst_imm;
	out.dest = detail::GetU<7, 11>(data);
	out.imm = (int64_t(detail::GetS<31, 31>(data)) << 20)
		| (uint64_t(detail::GetU<12, 19>(data)) << 12)
		| (uint64_t(detail::GetU<20, 20>(data)) << 11)
		| (uint64_t(detail::GetU<21, 30>(data)) << 1);

	out.opcode = rv64::Opcode::jump_and_link_imm;

	return out;
}

rv64::Instruction rv64::Decode(uint32_t data) {
	/* handle the separate opcodes and decode the corresponding instruction format */
	switch (detail::GetU<0, 6>(data)) {
	case 0x13:
		return detail::Opcode13(data);
	case 0x17:
		return detail::Opcode13(data);
	case 0x33:
		return detail::Opcode33(data);
	case 0x37:
		return detail::Opcode37(data);
	case 0x63:
		return detail::Opcode63(data);
	case 0x67:
		return detail::Opcode67(data);
	case 0x6f:
		return detail::Opcode6f(data);
	default:
		break;
	}
	return rv64::Instruction{};
}
