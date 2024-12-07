#include "rv64-decoder.h"

rv64::Instruction rv64::detail::Opcode03(uint32_t data) {
	rv64::Instruction out;

	out.dest = detail::GetU<7, 11>(data);
	out.src1 = detail::GetU<15, 19>(data);
	out.imm = detail::GetS<20, 31>(data);

	switch (detail::GetU<12, 14>(data)) {
	case 0x00:
		out.opcode = rv64::Opcode::load_byte_s;
		break;
	case 0x01:
		out.opcode = rv64::Opcode::load_half_s;
		break;
	case 0x02:
		out.opcode = rv64::Opcode::load_word_s;
		break;
	case 0x04:
		out.opcode = rv64::Opcode::load_byte_u;
		break;
	case 0x05:
		out.opcode = rv64::Opcode::load_half_u;
		break;
	}

	return out;
}
rv64::Instruction rv64::detail::Opcode13(uint32_t data) {
	rv64::Instruction out;

	out.dest = detail::GetU<7, 11>(data);
	out.src1 = detail::GetU<15, 19>(data);
	out.imm = detail::GetS<20, 31>(data);

	/* upper 7 bits (without the lowest bit, as it will be used for shifts due to rv64i) */
	uint32_t funct7 = (detail::GetU<26, 31>(data) << 1);

	switch (detail::GetU<12, 14>(data)) {
	case 0x00:
		out.opcode = rv64::Opcode::add_imm;

		/* special-case */
		if (out.imm == 0) {
			if (out.dest == 0 && out.src1 == 0)
				out.opcode = rv64::Opcode::no_op;
			else
				out.opcode = rv64::Opcode::move;
		}
		break;
	case 0x01:
		if (funct7 == 0x00)
			out.opcode = rv64::Opcode::shift_left_logic_imm;
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
		if (funct7 == 0x00)
			out.opcode = rv64::Opcode::shift_right_logic_imm;
		else if (funct7 == 0x20) {
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

	out.dest = detail::GetU<7, 11>(data);
	out.imm = int32_t(data & ~0x0fff);

	out.opcode = rv64::Opcode::add_upper_imm_pc;

	return out;
}
rv64::Instruction rv64::detail::Opcode23(uint32_t data) {
	rv64::Instruction out;

	out.src1 = detail::GetU<15, 19>(data);
	out.src2 = detail::GetU<20, 24>(data);
	out.imm = (int64_t(detail::GetS<25, 31>(data)) << 5)
		| uint64_t(detail::GetU<7, 11>(data));

	switch (detail::GetU<12, 14>(data)) {
	case 0x00:
		out.opcode = rv64::Opcode::store_byte;
		break;
	case 0x01:
		out.opcode = rv64::Opcode::store_half;
		break;
	case 0x02:
		out.opcode = rv64::Opcode::store_word;
		break;
	}

	return out;
}
rv64::Instruction rv64::detail::Opcode33(uint32_t data) {
	rv64::Instruction out;

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

	out.dest = detail::GetU<7, 11>(data);
	out.imm = int32_t(data & ~0x0fff);

	out.opcode = rv64::Opcode::load_upper_imm;

	return out;
}
rv64::Instruction rv64::detail::Opcode63(uint32_t data) {
	rv64::Instruction out;

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

	out.dest = detail::GetU<7, 11>(data);
	out.src1 = detail::GetU<15, 19>(data);
	out.imm = detail::GetS<20, 31>(data);

	if (detail::GetU<12, 14>(data) == 0x00)
		out.opcode = rv64::Opcode::jump_and_link_reg;

	return out;
}
rv64::Instruction rv64::detail::Opcode6f(uint32_t data) {
	rv64::Instruction out;

	out.dest = detail::GetU<7, 11>(data);
	out.imm = (int64_t(detail::GetS<31, 31>(data)) << 20)
		| (uint64_t(detail::GetU<12, 19>(data)) << 12)
		| (uint64_t(detail::GetU<20, 20>(data)) << 11)
		| (uint64_t(detail::GetU<21, 30>(data)) << 1);

	out.opcode = rv64::Opcode::jump_and_link_imm;

	return out;
}

rv64::Instruction rv64::detail::Quadrant0(uint16_t data) {
	rv64::Instruction out;
	out.compressed = true;

	/* handle all of the different instructions of the first quadrant */
	switch (detail::GetU<13, 15>(data)) {
	case 0x00:
		/* c.addi4spn */
		if (data != 0) {
			out.opcode = rv64::Opcode::add_imm;
			out.src1 = rv64::reg::X2;
			out.dest = detail::RVCRegisters[detail::GetU<2, 4>(data)];
			out.imm = (uint64_t(detail::GetU<7, 10>(data)) << 6)
				| (uint64_t(detail::GetU<11, 12>(data)) << 4)
				| (uint64_t(detail::GetU<5, 5>(data)) << 3)
				| (uint64_t(detail::GetU<6, 6>(data)) << 2);
		}
		break;
	case 0x02:
		/* c.lw */
		out.opcode = rv64::Opcode::load_word_s;
		out.src1 = detail::RVCRegisters[detail::GetU<7, 9>(data)];
		out.dest = detail::RVCRegisters[detail::GetU<2, 4>(data)];
		out.imm = (uint64_t(detail::GetU<5, 5>(data)) << 6)
			| (uint64_t(detail::GetU<10, 12>(data)) << 3)
			| (uint64_t(detail::GetU<6, 6>(data)) << 2);
		break;
	case 0x06:
		/* c.sw */
		out.opcode = rv64::Opcode::store_word;
		out.src1 = detail::RVCRegisters[detail::GetU<7, 9>(data)];
		out.src2 = detail::RVCRegisters[detail::GetU<2, 4>(data)];
		out.imm = (uint64_t(detail::GetU<5, 5>(data)) << 6)
			| (uint64_t(detail::GetU<10, 12>(data)) << 3)
			| (uint64_t(detail::GetU<6, 6>(data)) << 2);
		break;
	}

	return out;
}
rv64::Instruction rv64::detail::Quadrant1(uint16_t data) {
	rv64::Instruction out;
	out.compressed = true;

	/* handle all of the different instructions of the first quadrant */
	uint8_t code = detail::GetU<13, 15>(data);
	switch (code) {
	case 0x00:
		/* c.nop */
		if (data == 1)
			out.opcode = rv64::Opcode::no_op;

		/* c.addi */
		else {
			out.src1 = detail::GetU<7, 11>(data);
			out.dest = out.src1;
			out.imm = (int64_t(detail::GetS<12, 12>(data)) << 5)
				| (uint64_t(detail::GetU<2, 6>(data)) << 0);
			if (out.src1 != 0 && out.imm != 0)
				out.opcode = rv64::Opcode::add_imm;
		}
		break;
	case 0x02:
		/* c.li */
		out.src1 = rv64::reg::Zero;
		out.dest = detail::GetU<7, 11>(data);
		out.imm = (int64_t(detail::GetS<12, 12>(data)) << 5)
			| (uint64_t(detail::GetU<2, 6>(data)) << 0);
		if (out.dest != 0)
			out.opcode = rv64::Opcode::add_imm;
		break;
	case 0x03:
		out.dest = detail::GetU<7, 11>(data);

		/* c.addi16sp */
		if (out.dest == 2) {
			out.src1 = rv64::reg::X2;
			out.imm = (int64_t(detail::GetS<12, 12>(data)) << 9)
				| (uint64_t(detail::GetU<3, 4>(data)) << 7)
				| (uint64_t(detail::GetU<5, 5>(data)) << 6)
				| (uint64_t(detail::GetU<2, 2>(data)) << 5)
				| (uint64_t(detail::GetU<6, 6>(data)) << 4);
			if (out.imm != 0)
				out.opcode = rv64::Opcode::add_imm;
		}

		/* c.lui */
		else if (out.dest != 0) {
			out.imm = (int64_t(detail::GetS<12, 12>(data)) << 17)
				| (uint64_t(detail::GetU<2, 6>(data)) << 12);
			out.opcode = rv64::Opcode::load_upper_imm;
		}
		break;
	case 0x04:
		out.src1 = detail::RVCRegisters[detail::GetU<7, 9>(data)];
		out.dest = out.src1;

		switch (detail::GetU<10, 11>(data)) {
		case 0x00:
			/* c.srli */
			out.imm = (uint64_t(detail::GetU<12, 12>(data)) << 5)
				| (uint64_t(detail::GetU<2, 6>(data)) << 0);
			if (out.imm != 0)
				out.opcode = rv64::Opcode::shift_right_logic_imm;
			break;
		case 0x01:
			/* c.srai */
			out.imm = (uint64_t(detail::GetU<12, 12>(data)) << 5)
				| (uint64_t(detail::GetU<2, 6>(data)) << 0);
			if (out.imm != 0)
				out.opcode = rv64::Opcode::shift_right_arith_imm;
			break;
		case 0x02:
			/* c.andi */
			out.imm = (int64_t(detail::GetS<12, 12>(data)) << 5)
				| (uint64_t(detail::GetU<2, 6>(data)) << 0);
			out.opcode = rv64::Opcode::and_imm;
			break;
		case 0x03:
			out.src2 = detail::RVCRegisters[detail::GetU<2, 4>(data)];

			switch (detail::GetU<5, 6>(data)) {
			case 0x00:
				/* c.sub */
				out.opcode = rv64::Opcode::sub_reg;
				break;
			case 0x01:
				/* c.xor */
				out.opcode = rv64::Opcode::xor_reg;
				break;
			case 0x02:
				/* c.or */
				out.opcode = rv64::Opcode::or_reg;
				break;
			case 0x03:
				/* c.and */
				out.opcode = rv64::Opcode::and_reg;
				break;
			}
			break;
		}
		break;
	case 0x05:
		/* c.j */
		out.imm = (int64_t(detail::GetS<12, 12>(data)) << 11)
			| (uint64_t(detail::GetU<8, 8>(data)) << 10)
			| (uint64_t(detail::GetU<9, 10>(data)) << 8)
			| (uint64_t(detail::GetU<6, 6>(data)) << 7)
			| (uint64_t(detail::GetU<7, 7>(data)) << 6)
			| (uint64_t(detail::GetU<2, 2>(data)) << 5)
			| (uint64_t(detail::GetU<11, 11>(data)) << 4)
			| (uint64_t(detail::GetU<3, 5>(data)) << 1);
		out.opcode = rv64::Opcode::jump_and_link_reg;
		out.dest = rv64::reg::Zero;
		break;
	case 0x06:
	case 0x07:
		out.src1 = detail::RVCRegisters[detail::GetU<7, 9>(data)];
		out.imm = (int64_t(detail::GetS<12, 12>(data)) << 8)
			| (uint64_t(detail::GetU<5, 6>(data)) << 6)
			| (uint64_t(detail::GetU<2, 2>(data)) << 5)
			| (uint64_t(detail::GetU<10, 11>(data)) << 3)
			| (uint64_t(detail::GetU<3, 4>(data)) << 1);

		/* c.beqz */
		if (code) {
			out.src2 = rv64::reg::Zero;
			out.opcode = rv64::Opcode::branch_eq;
		}

		/* c.bnez */
		else {
			out.src2 = rv64::reg::Zero;
			out.opcode = rv64::Opcode::branch_ne;
		}
		break;
	}

	return out;
}
rv64::Instruction rv64::detail::Quadrant2(uint16_t data) {
	rv64::Instruction out;
	out.compressed = true;

	/* handle all of the different instructions of the first quadrant */
	uint8_t code = detail::GetU<13, 15>(data);
	switch (code) {
	case 0x00:
		/* c.slli */
		out.src1 = detail::GetU<7, 11>(data);
		out.dest = out.src1;
		out.imm = (uint64_t(detail::GetU<12, 12>(data)) << 5)
			| (uint64_t(detail::GetU<2, 6>(data)) << 0);
		if (out.src1 != 0 && out.imm != 0)
			out.opcode = rv64::Opcode::shift_left_logic_imm;
		break;
	case 0x02:
		/* c.lwsp */
		out.src1 = rv64::reg::X2;
		out.dest = detail::GetU<7, 11>(data);
		out.imm = (uint64_t(detail::GetU<2, 3>(data)) << 6)
			| (uint64_t(detail::GetU<12, 12>(data)) << 5)
			| (uint64_t(detail::GetU<4, 6>(data)) << 2);
		if (out.dest != 0)
			out.opcode = rv64::Opcode::load_word_s;
		break;
	case 0x04:
		out.src1 = detail::GetU<7, 11>(data);
		out.src2 = detail::GetU<2, 6>(data);

		if (detail::GetU<12, 12>(data) == 0) {
			/* c.jr */
			if (out.src1 != 0 && out.src2 == 0) {
				out.opcode = rv64::Opcode::jump_and_link_reg;
				out.dest = rv64::reg::Zero;
			}

			/* c.mv */
			else if (out.src1 != 0 && out.src2 != 0) {
				out.opcode = rv64::Opcode::move;
				out.dest = out.src1;
				out.src1 = out.src2;
			}
		}
		else if (out.src1 != 0) {
			/* c.jalr */
			if (out.src2 == 0) {
				out.opcode = rv64::Opcode::jump_and_link_reg;
				out.dest = rv64::reg::X1;
			}

			/* c.add */
			else {
				out.opcode = rv64::Opcode::add_reg;
				out.dest = out.src1;
			}
		}
		break;
	case 0x06:
		/* c.swsp */
		out.src1 = rv64::reg::X2;
		out.src2 = detail::GetU<2, 6>(data);
		out.imm = (uint64_t(detail::GetU<7, 8>(data)) << 6)
			| (uint64_t(detail::GetU<9, 12>(data)) << 2);
		out.opcode = rv64::Opcode::store_word;
		break;
	}

	return out;
}

rv64::Instruction rv64::Decode32(uint32_t data) {
	/* handle the separate opcodes and decode the corresponding instruction format */
	switch (detail::GetU<0, 6>(data)) {
	case 0x03:
		return detail::Opcode03(data);
	case 0x13:
		return detail::Opcode13(data);
	case 0x17:
		return detail::Opcode17(data);
	case 0x23:
		return detail::Opcode23(data);
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
rv64::Instruction rv64::Decode16(uint16_t data) {
	/* handle the corresponding compressed rv quadrant */
	switch (detail::GetU<0, 1>(data)) {
	case 0x00:
		return detail::Quadrant0(data);
	case 0x01:
		return detail::Quadrant1(data);
	case 0x02:
		return detail::Quadrant2(data);
	default:
		break;
	}
	return rv64::Instruction{};
}
