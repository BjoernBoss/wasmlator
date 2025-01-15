#include "rv64-print.h"

enum class FormatType : uint8_t {
	none,
	dst_imm,
	dst_src1,
	dst_src1_imm,
	dst_src1_src2,
	load,
	store,
	branch,
	jal,
	jalr,
	fence,
	dst_src1_csr,
	dst_imm_csr,
	dst_src1_amo,
	dst_src1_src2_amo,

	flt_load,
	flt_store,
	flt_dest_src1,
	flt_dest_src1_src2,
	flt_dest_src1_src2_src3,

	int_dest_flt_src1,
	flt_dest_int_src1
};
struct PrintOpcode {
	const char8_t* string = 0;
	FormatType format = FormatType::none;
};

static constexpr PrintOpcode opcodeStrings[] = {
	PrintOpcode{ u8"$misaligned", FormatType::none },

	PrintOpcode{ u8"lui", FormatType::dst_imm },
	PrintOpcode{ u8"auipc", FormatType::dst_imm },
	PrintOpcode{ u8"jal", FormatType::jal },
	PrintOpcode{ u8"jalr", FormatType::jalr },
	PrintOpcode{ u8"beq", FormatType::branch },
	PrintOpcode{ u8"bne", FormatType::branch },
	PrintOpcode{ u8"blt", FormatType::branch },
	PrintOpcode{ u8"bge", FormatType::branch },
	PrintOpcode{ u8"bltu", FormatType::branch },
	PrintOpcode{ u8"bgeu", FormatType::branch },

	PrintOpcode{ u8"lb", FormatType::load },
	PrintOpcode{ u8"lh", FormatType::load },
	PrintOpcode{ u8"lw", FormatType::load },
	PrintOpcode{ u8"lbu", FormatType::load },
	PrintOpcode{ u8"lhu", FormatType::load },
	PrintOpcode{ u8"lwu", FormatType::load },
	PrintOpcode{ u8"ld", FormatType::load },
	PrintOpcode{ u8"sb", FormatType::store },
	PrintOpcode{ u8"sh", FormatType::store },
	PrintOpcode{ u8"sw", FormatType::store },
	PrintOpcode{ u8"sd", FormatType::store },

	PrintOpcode{ u8"addi", FormatType::dst_src1_imm },
	PrintOpcode{ u8"addiw", FormatType::dst_src1_imm },
	PrintOpcode{ u8"xori", FormatType::dst_src1_imm },
	PrintOpcode{ u8"ori", FormatType::dst_src1_imm },
	PrintOpcode{ u8"andi", FormatType::dst_src1_imm },
	PrintOpcode{ u8"slli", FormatType::dst_src1_imm },
	PrintOpcode{ u8"slliw", FormatType::dst_src1_imm },
	PrintOpcode{ u8"srli", FormatType::dst_src1_imm },
	PrintOpcode{ u8"srliw", FormatType::dst_src1_imm },
	PrintOpcode{ u8"srai", FormatType::dst_src1_imm },
	PrintOpcode{ u8"sraiw", FormatType::dst_src1_imm },
	PrintOpcode{ u8"slti", FormatType::dst_src1_imm },
	PrintOpcode{ u8"sltiu", FormatType::dst_src1_imm },
	PrintOpcode{ u8"add", FormatType::dst_src1_src2 },
	PrintOpcode{ u8"addw", FormatType::dst_src1_src2 },
	PrintOpcode{ u8"sub", FormatType::dst_src1_src2 },
	PrintOpcode{ u8"subw", FormatType::dst_src1_src2 },
	PrintOpcode{ u8"xor", FormatType::dst_src1_src2 },
	PrintOpcode{ u8"or", FormatType::dst_src1_src2 },
	PrintOpcode{ u8"and", FormatType::dst_src1_src2 },
	PrintOpcode{ u8"sll", FormatType::dst_src1_src2 },
	PrintOpcode{ u8"sllw", FormatType::dst_src1_src2 },
	PrintOpcode{ u8"srl", FormatType::dst_src1_src2 },
	PrintOpcode{ u8"srlw", FormatType::dst_src1_src2 },
	PrintOpcode{ u8"sra", FormatType::dst_src1_src2 },
	PrintOpcode{ u8"sraw", FormatType::dst_src1_src2 },
	PrintOpcode{ u8"slt", FormatType::dst_src1_src2 },
	PrintOpcode{ u8"sltu", FormatType::dst_src1_src2 },

	PrintOpcode{ u8"fence", FormatType::fence },
	PrintOpcode{ u8"fence.i", FormatType::none },
	PrintOpcode{ u8"ebreak", FormatType::none },
	PrintOpcode{ u8"ecall", FormatType::none },
	PrintOpcode{ u8"csrrw", FormatType::dst_src1_csr },
	PrintOpcode{ u8"csrrs", FormatType::dst_src1_csr },
	PrintOpcode{ u8"csrrc", FormatType::dst_src1_csr },
	PrintOpcode{ u8"csrrwi", FormatType::dst_imm_csr },
	PrintOpcode{ u8"csrrsi", FormatType::dst_imm_csr },
	PrintOpcode{ u8"csrrci", FormatType::dst_imm_csr },

	PrintOpcode{ u8"mul", FormatType::dst_src1_src2 },
	PrintOpcode{ u8"mulw", FormatType::dst_src1_src2 },
	PrintOpcode{ u8"mulh", FormatType::dst_src1_src2 },
	PrintOpcode{ u8"mulhsu", FormatType::dst_src1_src2 },
	PrintOpcode{ u8"mulhu", FormatType::dst_src1_src2 },
	PrintOpcode{ u8"div", FormatType::dst_src1_src2 },
	PrintOpcode{ u8"divw", FormatType::dst_src1_src2 },
	PrintOpcode{ u8"divu", FormatType::dst_src1_src2 },
	PrintOpcode{ u8"divuw", FormatType::dst_src1_src2 },
	PrintOpcode{ u8"rem", FormatType::dst_src1_src2 },
	PrintOpcode{ u8"remw", FormatType::dst_src1_src2 },
	PrintOpcode{ u8"remu", FormatType::dst_src1_src2 },
	PrintOpcode{ u8"remuw", FormatType::dst_src1_src2 },

	PrintOpcode{ u8"lr.w", FormatType::dst_src1_amo },
	PrintOpcode{ u8"sc.w", FormatType::dst_src1_src2_amo },
	PrintOpcode{ u8"amoswap.w", FormatType::dst_src1_src2_amo },
	PrintOpcode{ u8"amoadd.w", FormatType::dst_src1_src2_amo },
	PrintOpcode{ u8"amoxor.w", FormatType::dst_src1_src2_amo },
	PrintOpcode{ u8"amoand.w", FormatType::dst_src1_src2_amo },
	PrintOpcode{ u8"amoor.w", FormatType::dst_src1_src2_amo },
	PrintOpcode{ u8"amomin.w", FormatType::dst_src1_src2_amo },
	PrintOpcode{ u8"amomax.w", FormatType::dst_src1_src2_amo },
	PrintOpcode{ u8"amominu.w", FormatType::dst_src1_src2_amo },
	PrintOpcode{ u8"amomaxu.w", FormatType::dst_src1_src2_amo },
	PrintOpcode{ u8"lr.d", FormatType::dst_src1_amo },
	PrintOpcode{ u8"sc.d", FormatType::dst_src1_src2_amo },
	PrintOpcode{ u8"amoswap.d", FormatType::dst_src1_src2_amo },
	PrintOpcode{ u8"amoadd.d", FormatType::dst_src1_src2_amo },
	PrintOpcode{ u8"amoxor.d", FormatType::dst_src1_src2_amo },
	PrintOpcode{ u8"amoand.d", FormatType::dst_src1_src2_amo },
	PrintOpcode{ u8"amoor.d", FormatType::dst_src1_src2_amo },
	PrintOpcode{ u8"amomin.d", FormatType::dst_src1_src2_amo },
	PrintOpcode{ u8"amomax.d", FormatType::dst_src1_src2_amo },
	PrintOpcode{ u8"amominu.d", FormatType::dst_src1_src2_amo },
	PrintOpcode{ u8"amomaxu.d", FormatType::dst_src1_src2_amo },

	PrintOpcode{ u8"flw", FormatType::flt_load },
	PrintOpcode{ u8"fld", FormatType::flt_load },
	PrintOpcode{ u8"fsw", FormatType::flt_store },
	PrintOpcode{ u8"fsd", FormatType::flt_store },

	PrintOpcode{ u8"fmadd.s", FormatType::flt_dest_src1_src2_src3 },
	PrintOpcode{ u8"fmsub.s", FormatType::flt_dest_src1_src2_src3 },
	PrintOpcode{ u8"fnmadd.s", FormatType::flt_dest_src1_src2_src3 },
	PrintOpcode{ u8"fnmsub.s", FormatType::flt_dest_src1_src2_src3 },
	PrintOpcode{ u8"fadd.s", FormatType::flt_dest_src1_src2 },
	PrintOpcode{ u8"fsub.s", FormatType::flt_dest_src1_src2 },
	PrintOpcode{ u8"fmul.s", FormatType::flt_dest_src1_src2 },
	PrintOpcode{ u8"fdiv.s", FormatType::flt_dest_src1_src2 },
	PrintOpcode{ u8"fsqrt.s", FormatType::flt_dest_src1 },
	PrintOpcode{ u8"fsgnj.s", FormatType::flt_dest_src1_src2 },
	PrintOpcode{ u8"fsgnjn.s", FormatType::flt_dest_src1_src2 },
	PrintOpcode{ u8"fsgnjx.s", FormatType::flt_dest_src1_src2 },
	PrintOpcode{ u8"fmin.s", FormatType::flt_dest_src1_src2 },
	PrintOpcode{ u8"fmax.s", FormatType::flt_dest_src1_src2 },
	PrintOpcode{ u8"fle.s", FormatType::flt_dest_src1_src2 },
	PrintOpcode{ u8"flt.s", FormatType::flt_dest_src1_src2 },
	PrintOpcode{ u8"feq.s", FormatType::flt_dest_src1_src2 },
	PrintOpcode{ u8"fclass.s", FormatType::int_dest_flt_src1 },

	PrintOpcode{ u8"fmadd.d", FormatType::flt_dest_src1_src2_src3 },
	PrintOpcode{ u8"fmsub.d", FormatType::flt_dest_src1_src2_src3 },
	PrintOpcode{ u8"fnmadd.d", FormatType::flt_dest_src1_src2_src3 },
	PrintOpcode{ u8"fnmsub.d", FormatType::flt_dest_src1_src2_src3 },
	PrintOpcode{ u8"fadd.d", FormatType::flt_dest_src1_src2 },
	PrintOpcode{ u8"fsub.d", FormatType::flt_dest_src1_src2 },
	PrintOpcode{ u8"fmul.d", FormatType::flt_dest_src1_src2 },
	PrintOpcode{ u8"fdiv.d", FormatType::flt_dest_src1_src2 },
	PrintOpcode{ u8"fsqrt.d", FormatType::flt_dest_src1 },
	PrintOpcode{ u8"fsgnj.d", FormatType::flt_dest_src1_src2 },
	PrintOpcode{ u8"fsgnjn.d", FormatType::flt_dest_src1_src2 },
	PrintOpcode{ u8"fsgnjx.d", FormatType::flt_dest_src1_src2 },
	PrintOpcode{ u8"fmin.d", FormatType::flt_dest_src1_src2 },
	PrintOpcode{ u8"fmax.d", FormatType::flt_dest_src1_src2 },
	PrintOpcode{ u8"fle.d", FormatType::flt_dest_src1_src2 },
	PrintOpcode{ u8"flt.d", FormatType::flt_dest_src1_src2 },
	PrintOpcode{ u8"feq.d", FormatType::flt_dest_src1_src2 },
	PrintOpcode{ u8"fclass.d", FormatType::int_dest_flt_src1 },

	PrintOpcode{ u8"fcvt.s.w", FormatType::int_dest_flt_src1 },
	PrintOpcode{ u8"fcvt.s.wu", FormatType::int_dest_flt_src1 },
	PrintOpcode{ u8"fcvt.w.s", FormatType::flt_dest_int_src1 },
	PrintOpcode{ u8"fcvt.wu.s", FormatType::flt_dest_int_src1 },
	PrintOpcode{ u8"fcvt.s.l", FormatType::int_dest_flt_src1 },
	PrintOpcode{ u8"fcvt.s.lu", FormatType::int_dest_flt_src1 },
	PrintOpcode{ u8"fcvt.l.s", FormatType::flt_dest_int_src1 },
	PrintOpcode{ u8"fcvt.lu.s", FormatType::flt_dest_int_src1 },
	PrintOpcode{ u8"fmv.x.w", FormatType::int_dest_flt_src1 },
	PrintOpcode{ u8"fmv.w.x", FormatType::flt_dest_int_src1 },

	PrintOpcode{ u8"fcvt.d.w", FormatType::int_dest_flt_src1 },
	PrintOpcode{ u8"fcvt.d.wu", FormatType::int_dest_flt_src1 },
	PrintOpcode{ u8"fcvt.w.d", FormatType::flt_dest_int_src1 },
	PrintOpcode{ u8"fcvt.wu.d", FormatType::flt_dest_int_src1 },
	PrintOpcode{ u8"fcvt.d.l", FormatType::int_dest_flt_src1 },
	PrintOpcode{ u8"fcvt.d.lu", FormatType::int_dest_flt_src1 },
	PrintOpcode{ u8"fcvt.l.d", FormatType::flt_dest_int_src1 },
	PrintOpcode{ u8"fcvt.lu.d", FormatType::flt_dest_int_src1 },
	PrintOpcode{ u8"fmv.x.d", FormatType::int_dest_flt_src1 },
	PrintOpcode{ u8"fmv.d.x", FormatType::flt_dest_int_src1 },

	PrintOpcode{ u8"fcvt.s.d", FormatType::flt_dest_src1 },
	PrintOpcode{ u8"fcvt.d.s", FormatType::flt_dest_src1 },
};
static constexpr const char8_t* iRegisters[] = {
	u8"zero", u8"ra", u8"sp", u8"gp", u8"tp", u8"t0", u8"t1", u8"t2",
	u8"fp", u8"s1", u8"a0", u8"a1", u8"a2", u8"a3", u8"a4", u8"a5",
	u8"a6", u8"a7", u8"s2", u8"s3", u8"s4", u8"s5", u8"s6", u8"s7",
	u8"s8", u8"s9", u8"s10", u8"s11", u8"t3", u8"t4", u8"t5", u8"t6"
};
static constexpr const char8_t* fRegisters[] = {
	u8"ft0", u8"ft1", u8"ft2", u8"ft3", u8"ft4", u8"ft5", u8"ft6", u8"ft7",
	u8"fs0", u8"fs1", u8"fa0", u8"fa1", u8"fa2", u8"fa3", u8"fa4", u8"fa5",
	u8"fa6", u8"fa7", u8"fs2", u8"fs3", u8"fs4", u8"fs5", u8"fs6", u8"fs7",
	u8"fs8", u8"fs9", u8"fs10", u8"fs11", u8"ft8", u8"ft9", u8"ft10", u8"ft11"
};

std::u8string rv64::ToString(const rv64::Instruction& inst) {
	static_assert(sizeof(opcodeStrings) / sizeof(PrintOpcode) == size_t(rv64::Opcode::_invalid), "string-table and opcode-count must match");
	static_assert(sizeof(iRegisters) / sizeof(const char8_t*) == 32, "string-table must provide string for all 32 general-purpose registers");
	static_assert(sizeof(fRegisters) / sizeof(const char8_t*) == 32, "string-table must provide string for all 32 floating-point registers");

	/* check if the instruction is not valid, in which case format and other operands are irrelevant */
	if (inst.opcode == rv64::Opcode::_invalid)
		return u8"$invalid_instruction";

	/* add the operands */
	PrintOpcode opcode = opcodeStrings[size_t(inst.opcode)];
	std::u8string out = str::u8::Build(((inst.size == 2) ? u8"c." : u8""), opcode.string);
	switch (opcode.format) {
	case FormatType::dst_src1:
		str::BuildTo(out, u8' ', iRegisters[inst.dest], u8", ", iRegisters[inst.src1]);
		break;
	case FormatType::dst_src1_imm:
		str::BuildTo(out, u8' ', iRegisters[inst.dest], u8", ", iRegisters[inst.src1], u8", ", inst.imm);
		break;
	case FormatType::dst_src1_src2:
		str::BuildTo(out, u8' ', iRegisters[inst.dest], u8", ", iRegisters[inst.src1], u8", ", iRegisters[inst.src2]);
		break;
	case FormatType::dst_imm:
		str::BuildTo(out, u8' ', iRegisters[inst.dest], u8", ", inst.imm);
		break;
	case FormatType::load:
		str::BuildTo(out, u8' ', iRegisters[inst.dest], u8", [", iRegisters[inst.src1], u8" + ", inst.imm, u8']');
		break;
	case FormatType::store:
		str::BuildTo(out, u8" [", iRegisters[inst.src1], u8" + ", inst.imm, u8"], ", iRegisters[inst.src2]);
		break;
	case FormatType::branch:
		str::BuildTo(out, u8' ', iRegisters[inst.src1], u8", ", iRegisters[inst.src2], u8", $(pc + ", inst.imm, u8')');
		break;
	case FormatType::jal:
		str::BuildTo(out, u8' ', iRegisters[inst.dest], u8", $(pc + ", inst.imm, u8')');
		break;
	case FormatType::jalr:
		str::BuildTo(out, u8' ', iRegisters[inst.dest], u8", $(", iRegisters[inst.src1], u8" + ", inst.imm, u8')');
		break;
	case FormatType::fence:
		str::FormatTo(out, u8" (pred: {:04b} | succ: {:04b})", (inst.misc >> 4) & 0x0f, inst.misc & 0x0f);
		break;
	case FormatType::dst_src1_csr:
		str::FormatTo(out, u8" {}, {} csr:{:#05x}", iRegisters[inst.dest], iRegisters[inst.src1], inst.misc);
		break;
	case FormatType::dst_imm_csr:
		str::FormatTo(out, u8" {}, {} csr:{:#05x}", iRegisters[inst.dest], inst.imm, inst.misc);
		break;
	case FormatType::dst_src1_amo:
		str::BuildTo(out, ((inst.misc & 0x02) != 0 ? u8".aq" : u8""), ((inst.misc & 0x01) != 0 ? u8".rl" : u8""),
			u8' ', iRegisters[inst.dest], u8", [", iRegisters[inst.src1], u8']');
		break;
	case FormatType::dst_src1_src2_amo:
		str::BuildTo(out, ((inst.misc & 0x02) != 0 ? u8".aq" : u8""), ((inst.misc & 0x01) != 0 ? u8".rl" : u8""),
			u8' ', iRegisters[inst.dest], u8", [", iRegisters[inst.src1], u8"], ", iRegisters[inst.src2]);
		break;
	case FormatType::flt_load:
		str::BuildTo(out, u8' ', fRegisters[inst.dest], u8", [", iRegisters[inst.src1], u8" + ", inst.imm, u8']');
		break;
	case FormatType::flt_store:
		str::BuildTo(out, u8" [", iRegisters[inst.src1], u8" + ", inst.imm, u8"], ", fRegisters[inst.src2]);
		break;
	case FormatType::flt_dest_src1:
		str::BuildTo(out, u8' ', fRegisters[inst.dest], u8", ", fRegisters[inst.src1]);
		break;
	case FormatType::flt_dest_src1_src2:
		str::BuildTo(out, u8' ', fRegisters[inst.dest], u8", ", fRegisters[inst.src1], u8", ", fRegisters[inst.src2]);
		break;
	case FormatType::flt_dest_src1_src2_src3:
		str::BuildTo(out, u8' ', fRegisters[inst.dest], u8", ", fRegisters[inst.src1], u8", ", fRegisters[inst.src2], u8", ", fRegisters[inst.src3]);
		break;
	case FormatType::int_dest_flt_src1:
		str::BuildTo(out, u8' ', iRegisters[inst.dest], u8", ", fRegisters[inst.src1]);
		break;
	case FormatType::flt_dest_int_src1:
		str::BuildTo(out, u8' ', fRegisters[inst.dest], u8", ", iRegisters[inst.src1]);
		break;
	default:
		break;
	}
	return out;
}
