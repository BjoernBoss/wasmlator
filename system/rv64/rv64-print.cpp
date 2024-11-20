#include "rv64-print.h"

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

std::u8string rv64::ToString(const rv64::Instruction& inst) {
	static_assert(sizeof(opcodeStrings) / sizeof(const char8_t*) == size_t(rv64::Opcode::_invalid), "string-table and opcode-count must match");
	static_assert(sizeof(registerStrings) / sizeof(const char8_t*) == 32, "string-table must provide string for all 32 general-purpose registers");

	/* check if the instruction is not valid, in which case format and other operands are irrelevant */
	if (inst.opcode == rv64::Opcode::_invalid)
		return u8"$invalid_instruction";

	/* add the operands */
	std::u8string out = opcodeStrings[size_t(inst.opcode)];
	switch (inst.format) {
	case rv64::Format::dst_src1:
		str::BuildTo(out, u8' ', registerStrings[inst.dest], u8", ", registerStrings[inst.src1]);
		break;
	case rv64::Format::dst_src1_imm:
		str::BuildTo(out, u8' ', registerStrings[inst.dest], u8", ", registerStrings[inst.src1], u8", ", inst.imm);
		break;
	case rv64::Format::dst_src1_src2:
		str::BuildTo(out, u8' ', registerStrings[inst.dest], u8", ", registerStrings[inst.src1], u8", ", registerStrings[inst.src2]);
		break;
	case rv64::Format::dst_imm:
		str::BuildTo(out, u8' ', registerStrings[inst.dest], u8", ", inst.imm);
		break;
	case rv64::Format::src1_src2:
		str::BuildTo(out, u8' ', registerStrings[inst.src1], u8", ", registerStrings[inst.src2]);
		break;
	default:
		break;
	}
	return out;
}
