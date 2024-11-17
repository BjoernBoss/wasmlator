#include "rv64-decoder.h"

namespace dec {
	template <size_t First, size_t Last>
	uint32_t GetU(uint32_t data) {
		static constexpr size_t Count = (Last - First + 1);
		return (data >> First) & ((1 << Count) - 1);
	}

	template <size_t First, size_t Last>
	int32_t GetS(uint32_t data) {
		static constexpr size_t Count = (Last - First + 1);

		uint32_t v = dec::GetU<First, Last>(data);
		if ((v >> (Count - 1)) & 0x01)
			v |= (uint64_t(0xffffffff) << Count);
		return int32_t(v);
	}

	static rv64::Instruction Opcode13(uint32_t data) {
		rv64::Instruction out;

		out.format = rv64::Format::dst_src1_imm;
		out.regDest = dec::GetU<7, 11>(data);
		out.regSrc1 = dec::GetU<15, 19>(data);
		out.immediate = dec::GetS<20, 31>(data);

		/* RV64I */
		uint32_t funct6 = dec::GetU<26, 31>(data);

		switch (dec::GetU<12, 14>(data)) {
		case 0x00:
			out.opcode = rv64::Opcode::add_imm;

			/* special-case */
			if (out.immediate == 0) {
				if (out.regDest == 0 && out.regSrc1 == 0) {
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
				out.immediate &= 0x3f;
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
				out.immediate &= 0x3f;
			}
			else if (funct6 == 0x10) {
				out.opcode = rv64::Opcode::shift_right_arith_imm;
				out.immediate &= 0x3f;
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
	static rv64::Instruction Opcode17(uint32_t data) {
		rv64::Instruction out;

		out.format = rv64::Format::dst_imm;
		out.regDest = dec::GetU<7, 11>(data);
		out.immediate = int32_t(data & ~0x0fff);

		out.opcode = rv64::Opcode::add_upper_imm_pc;

		return out;
	}
	static rv64::Instruction Opcode33(uint32_t data) {
		rv64::Instruction out;

		out.format = rv64::Format::dst_src1_src2;
		out.regDest = dec::GetU<7, 11>(data);
		out.regSrc1 = dec::GetU<15, 19>(data);
		out.regSrc2 = dec::GetU<20, 24>(data);

		uint32_t funct7 = dec::GetU<25, 31>(data);

		switch (dec::GetU<12, 14>(data)) {
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
	static rv64::Instruction Opcode37(uint32_t data) {
		rv64::Instruction out;

		out.format = rv64::Format::dst_imm;
		out.regDest = dec::GetU<7, 11>(data);
		out.immediate = int32_t(data & ~0x0fff);

		out.opcode = rv64::Opcode::load_upper_imm;

		return out;
	}
	static rv64::Instruction Opcode63(uint32_t data) {
		rv64::Instruction out;

		out.format = rv64::Format::src1_src2;
		out.regSrc1 = dec::GetU<15, 19>(data);
		out.regSrc2 = dec::GetU<20, 24>(data);
		out.immediate = (int64_t(dec::GetS<31, 31>(data)) << 12)
			| (uint64_t(dec::GetU<7, 7>(data)) << 11)
			| (uint64_t(dec::GetU<25, 30>(data)) << 5)
			| (uint64_t(dec::GetU<8, 11>(data)) << 1);

		switch (dec::GetU<12, 14>(data)) {
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
	static rv64::Instruction Opcode67(uint32_t data) {
		rv64::Instruction out;

		out.format = rv64::Format::dst_src1_imm;
		out.regDest = dec::GetU<7, 11>(data);
		out.regSrc1 = dec::GetU<15, 19>(data);
		out.immediate = dec::GetS<20, 31>(data);

		if (dec::GetU<12, 14>(data) == 0x00)
			out.opcode = rv64::Opcode::jump_and_link_reg;

		return out;
	}
	static rv64::Instruction Opcode6f(uint32_t data) {
		rv64::Instruction out;

		out.format = rv64::Format::dst_imm;
		out.regDest = dec::GetU<7, 11>(data);
		out.immediate = (int64_t(dec::GetS<31, 31>(data)) << 20)
			| (uint64_t(dec::GetU<12, 19>(data)) << 12)
			| (uint64_t(dec::GetU<20, 20>(data)) << 11)
			| (uint64_t(dec::GetU<21, 30>(data)) << 1);

		out.opcode = rv64::Opcode::jump_and_link_imm;

		return out;
	}
}

rv64::Instruction rv64::Decode(uint32_t data) {
	/* handle the separate opcodes and decode the corresponding instruction format */
	switch (dec::GetU<0, 6>(data)) {
	case 0x13:
		return dec::Opcode13(data);
	case 0x17:
		return dec::Opcode13(data);
	case 0x33:
		return dec::Opcode33(data);
	case 0x37:
		return dec::Opcode37(data);
	case 0x63:
		return dec::Opcode63(data);
	case 0x67:
		return dec::Opcode67(data);
	case 0x6f:
		return dec::Opcode6f(data);
	default:
		break;
	}
	return rv64::Instruction{ rv64::Opcode::_invalid, rv64::Format::none, 0, 0, 0, 0 };
}

std::u8string rv64::ToString(const rv64::Instruction& inst) {
	static constexpr const char8_t* opcodeStrings[] = {
		u8"lui",
		u8"auipc",
		u8"jal",
		u8"jalr",
		u8"beq",
		u8"bne",
		u8"blt",
		u8"bge",
		u8"bltu",
		u8"bgeu",
		u8"addi",
		u8"slti",
		u8"sltiu",
		u8"xori",
		u8"ori",
		u8"andi",
		u8"slli",
		u8"srli",
		u8"srai",
		u8"add",
		u8"sub",
		u8"mul",
		u8"mulh",
		u8"mulhsu",
		u8"mulhu",
		u8"div",
		u8"divu",
		u8"rem",
		u8"remu",
		u8"slt",
		u8"sltu",
		u8"xor",
		u8"or",
		u8"and",
		u8"sll",
		u8"srl",
		u8"sra",
		u8"nop",
		u8"mv",
	};
	static constexpr const char8_t* registerStrings[] = {
		u8"zero",
		u8"ra",
		u8"sp",
		u8"gp",
		u8"tp",
		u8"t0",
		u8"t1",
		u8"t2",
		u8"fp",
		u8"s1",
		u8"a0",
		u8"a1",
		u8"a2",
		u8"a3",
		u8"a4",
		u8"a5",
		u8"a6",
		u8"a7",
		u8"s2",
		u8"s3",
		u8"s4",
		u8"s5",
		u8"s6",
		u8"s7",
		u8"s8",
		u8"s9",
		u8"s10",
		u8"s11",
		u8"t3",
		u8"t4",
		u8"t6",
		u8"t6"
	};
	static_assert(sizeof(opcodeStrings) / sizeof(const char8_t*) == size_t(rv64::Opcode::_invalid), "string-table and opcode-count must match");
	static_assert(sizeof(registerStrings) / sizeof(const char8_t*) == 32, "string-table must provide string for all 32 general-purpose registers");

	/* check if the instruction is not valid, in which case format and other operands are irrelevant */
	if (inst.opcode == rv64::Opcode::_invalid)
		return u8"$invalid_instruction";

	/* add the operands */
	std::u8string out = opcodeStrings[size_t(inst.opcode)];
	switch (inst.format) {
	case rv64::Format::dst_src1:
		str::BuildTo(out, u8' ', registerStrings[inst.regDest], u8", ", registerStrings[inst.regSrc1]);
		break;
	case rv64::Format::dst_src1_imm:
		str::BuildTo(out, u8' ', registerStrings[inst.regDest], u8", ", registerStrings[inst.regSrc1], u8", ", inst.immediate);
		break;
	case rv64::Format::dst_src1_src2:
		str::BuildTo(out, u8' ', registerStrings[inst.regDest], u8", ", registerStrings[inst.regSrc1], u8", ", registerStrings[inst.regSrc2]);
		break;
	case rv64::Format::dst_imm:
		str::BuildTo(out, u8' ', registerStrings[inst.regDest], u8", ", inst.immediate);
		break;
	case rv64::Format::src1_src2:
		str::BuildTo(out, u8' ', registerStrings[inst.regSrc1], u8", ", registerStrings[inst.regSrc2]);
		break;
	default:
		break;
	}
	return out;
}
