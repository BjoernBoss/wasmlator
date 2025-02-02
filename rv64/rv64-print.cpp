#include "rv64-print.h"

enum class FormatType : uint8_t {
	none,
	fence,
	imx,
	s1i,
	dti,
	dti_imx,
	dti_jmp,
	dti_jlr,
	dti_s1i,
	dti_s2i,
	dti_mem,
	dti_csr,
	dtf_mem,
	dti_s1f,
	dtf_s1f,
	dtf_s1i,
	csr_s1i,
	csr_imx,
	s1i_jmp,
	s2i_mem,
	s2f_mem,
	s2i_jmp,
	dti_csr_s1i,
	dti_csr_imx,
	dti_imx_s1i,
	dtf_imx_s1i,
	dtf_s1f_s2f,
	dti_s1i_imd,
	dti_s1i_s2i,
	s1i_s2i_jmp,
	amo_dti_s1i,
	amo_dti_s1i_s2i,
	dtf_s1f_s2f_s3f
};
struct PrintOpcode {
	const char8_t* string = 0;
	FormatType format = FormatType::none;
};

static constexpr PrintOpcode opcodeStrings[] = {
	PrintOpcode{ u8"$misaligned_instruction", FormatType::none },
	PrintOpcode{ u8"$illegal_instruction", FormatType::none },

	PrintOpcode{ u8"li", FormatType::dti_imx },
	PrintOpcode{ u8"la", FormatType::dti_imx },

	PrintOpcode{ u8"lb", FormatType::dti_imx },
	PrintOpcode{ u8"lh", FormatType::dti_imx },
	PrintOpcode{ u8"lw", FormatType::dti_imx },
	PrintOpcode{ u8"lbu", FormatType::dti_imx },
	PrintOpcode{ u8"lhu", FormatType::dti_imx },
	PrintOpcode{ u8"lwu", FormatType::dti_imx },
	PrintOpcode{ u8"ld", FormatType::dti_imx },
	PrintOpcode{ u8"sb", FormatType::dti_imx_s1i },
	PrintOpcode{ u8"sh", FormatType::dti_imx_s1i },
	PrintOpcode{ u8"sw", FormatType::dti_imx_s1i },
	PrintOpcode{ u8"sd", FormatType::dti_imx_s1i },
	PrintOpcode{ u8"flw", FormatType::dtf_imx_s1i },
	PrintOpcode{ u8"fld", FormatType::dtf_imx_s1i },
	PrintOpcode{ u8"fsw", FormatType::dtf_imx_s1i },
	PrintOpcode{ u8"fsd", FormatType::dtf_imx_s1i },
	PrintOpcode{ u8"call", FormatType::imx },
	PrintOpcode{ u8"tail", FormatType::imx },

	PrintOpcode{ u8"nop", FormatType::none },
	PrintOpcode{ u8"lui", FormatType::dti_imx },
	PrintOpcode{ u8"auipc", FormatType::dti_imx },
	PrintOpcode{ u8"jal", FormatType::dti_jmp },
	PrintOpcode{ u8"jalr", FormatType::dti_jlr },
	PrintOpcode{ u8"beq", FormatType::s1i_s2i_jmp },
	PrintOpcode{ u8"bne", FormatType::s1i_s2i_jmp },
	PrintOpcode{ u8"blt", FormatType::s1i_s2i_jmp },
	PrintOpcode{ u8"bge", FormatType::s1i_s2i_jmp },
	PrintOpcode{ u8"bltu", FormatType::s1i_s2i_jmp },
	PrintOpcode{ u8"bgeu", FormatType::s1i_s2i_jmp },

	PrintOpcode{ u8"lb", FormatType::dti_mem },
	PrintOpcode{ u8"lh", FormatType::dti_mem },
	PrintOpcode{ u8"lw", FormatType::dti_mem },
	PrintOpcode{ u8"lbu", FormatType::dti_mem },
	PrintOpcode{ u8"lhu", FormatType::dti_mem },
	PrintOpcode{ u8"lwu", FormatType::dti_mem },
	PrintOpcode{ u8"ld", FormatType::dti_mem },
	PrintOpcode{ u8"sb", FormatType::s2i_mem },
	PrintOpcode{ u8"sh", FormatType::s2i_mem },
	PrintOpcode{ u8"sw", FormatType::s2i_mem },
	PrintOpcode{ u8"sd", FormatType::s2i_mem },

	PrintOpcode{ u8"addi", FormatType::dti_s1i_imd },
	PrintOpcode{ u8"addiw", FormatType::dti_s1i_imd },
	PrintOpcode{ u8"xori", FormatType::dti_s1i_imd },
	PrintOpcode{ u8"ori", FormatType::dti_s1i_imd },
	PrintOpcode{ u8"andi", FormatType::dti_s1i_imd },
	PrintOpcode{ u8"slli", FormatType::dti_s1i_imd },
	PrintOpcode{ u8"slliw", FormatType::dti_s1i_imd },
	PrintOpcode{ u8"srli", FormatType::dti_s1i_imd },
	PrintOpcode{ u8"srliw", FormatType::dti_s1i_imd },
	PrintOpcode{ u8"srai", FormatType::dti_s1i_imd },
	PrintOpcode{ u8"sraiw", FormatType::dti_s1i_imd },
	PrintOpcode{ u8"slti", FormatType::dti_s1i_imd },
	PrintOpcode{ u8"sltiu", FormatType::dti_s1i_imd },
	PrintOpcode{ u8"add", FormatType::dti_s1i_s2i },
	PrintOpcode{ u8"addw", FormatType::dti_s1i_s2i },
	PrintOpcode{ u8"sub", FormatType::dti_s1i_s2i },
	PrintOpcode{ u8"subw", FormatType::dti_s1i_s2i },
	PrintOpcode{ u8"xor", FormatType::dti_s1i_s2i },
	PrintOpcode{ u8"or", FormatType::dti_s1i_s2i },
	PrintOpcode{ u8"and", FormatType::dti_s1i_s2i },
	PrintOpcode{ u8"sll", FormatType::dti_s1i_s2i },
	PrintOpcode{ u8"sllw", FormatType::dti_s1i_s2i },
	PrintOpcode{ u8"srl", FormatType::dti_s1i_s2i },
	PrintOpcode{ u8"srlw", FormatType::dti_s1i_s2i },
	PrintOpcode{ u8"sra", FormatType::dti_s1i_s2i },
	PrintOpcode{ u8"sraw", FormatType::dti_s1i_s2i },
	PrintOpcode{ u8"slt", FormatType::dti_s1i_s2i },
	PrintOpcode{ u8"sltu", FormatType::dti_s1i_s2i },

	PrintOpcode{ u8"fence", FormatType::fence },
	PrintOpcode{ u8"fence.i", FormatType::none },
	PrintOpcode{ u8"ebreak", FormatType::none },
	PrintOpcode{ u8"ecall", FormatType::none },
	PrintOpcode{ u8"csrrw", FormatType::dti_csr_s1i },
	PrintOpcode{ u8"csrrs", FormatType::dti_csr_s1i },
	PrintOpcode{ u8"csrrc", FormatType::dti_csr_s1i },
	PrintOpcode{ u8"csrrwi", FormatType::dti_csr_imx },
	PrintOpcode{ u8"csrrsi", FormatType::dti_csr_imx },
	PrintOpcode{ u8"csrrci", FormatType::dti_csr_imx },

	PrintOpcode{ u8"mul", FormatType::dti_s1i_s2i },
	PrintOpcode{ u8"mulw", FormatType::dti_s1i_s2i },
	PrintOpcode{ u8"mulh", FormatType::dti_s1i_s2i },
	PrintOpcode{ u8"mulhsu", FormatType::dti_s1i_s2i },
	PrintOpcode{ u8"mulhu", FormatType::dti_s1i_s2i },
	PrintOpcode{ u8"div", FormatType::dti_s1i_s2i },
	PrintOpcode{ u8"divw", FormatType::dti_s1i_s2i },
	PrintOpcode{ u8"divu", FormatType::dti_s1i_s2i },
	PrintOpcode{ u8"divuw", FormatType::dti_s1i_s2i },
	PrintOpcode{ u8"rem", FormatType::dti_s1i_s2i },
	PrintOpcode{ u8"remw", FormatType::dti_s1i_s2i },
	PrintOpcode{ u8"remu", FormatType::dti_s1i_s2i },
	PrintOpcode{ u8"remuw", FormatType::dti_s1i_s2i },

	PrintOpcode{ u8"lr.w", FormatType::amo_dti_s1i },
	PrintOpcode{ u8"sc.w", FormatType::amo_dti_s1i_s2i },
	PrintOpcode{ u8"amoswap.w", FormatType::amo_dti_s1i_s2i },
	PrintOpcode{ u8"amoadd.w", FormatType::amo_dti_s1i_s2i },
	PrintOpcode{ u8"amoxor.w", FormatType::amo_dti_s1i_s2i },
	PrintOpcode{ u8"amoand.w", FormatType::amo_dti_s1i_s2i },
	PrintOpcode{ u8"amoor.w", FormatType::amo_dti_s1i_s2i },
	PrintOpcode{ u8"amomin.w", FormatType::amo_dti_s1i_s2i },
	PrintOpcode{ u8"amomax.w", FormatType::amo_dti_s1i_s2i },
	PrintOpcode{ u8"amominu.w", FormatType::amo_dti_s1i_s2i },
	PrintOpcode{ u8"amomaxu.w", FormatType::amo_dti_s1i_s2i },
	PrintOpcode{ u8"lr.d", FormatType::amo_dti_s1i },
	PrintOpcode{ u8"sc.d", FormatType::amo_dti_s1i_s2i },
	PrintOpcode{ u8"amoswap.d", FormatType::amo_dti_s1i_s2i },
	PrintOpcode{ u8"amoadd.d", FormatType::amo_dti_s1i_s2i },
	PrintOpcode{ u8"amoxor.d", FormatType::amo_dti_s1i_s2i },
	PrintOpcode{ u8"amoand.d", FormatType::amo_dti_s1i_s2i },
	PrintOpcode{ u8"amoor.d", FormatType::amo_dti_s1i_s2i },
	PrintOpcode{ u8"amomin.d", FormatType::amo_dti_s1i_s2i },
	PrintOpcode{ u8"amomax.d", FormatType::amo_dti_s1i_s2i },
	PrintOpcode{ u8"amominu.d", FormatType::amo_dti_s1i_s2i },
	PrintOpcode{ u8"amomaxu.d", FormatType::amo_dti_s1i_s2i },

	PrintOpcode{ u8"flw", FormatType::dtf_mem },
	PrintOpcode{ u8"fld", FormatType::dtf_mem },
	PrintOpcode{ u8"fsw", FormatType::s2f_mem },
	PrintOpcode{ u8"fsd", FormatType::s2f_mem },

	PrintOpcode{ u8"fmadd.s", FormatType::dtf_s1f_s2f_s3f },
	PrintOpcode{ u8"fmsub.s", FormatType::dtf_s1f_s2f_s3f },
	PrintOpcode{ u8"fnmadd.s", FormatType::dtf_s1f_s2f_s3f },
	PrintOpcode{ u8"fnmsub.s", FormatType::dtf_s1f_s2f_s3f },
	PrintOpcode{ u8"fadd.s", FormatType::dtf_s1f_s2f },
	PrintOpcode{ u8"fsub.s", FormatType::dtf_s1f_s2f },
	PrintOpcode{ u8"fmul.s", FormatType::dtf_s1f_s2f },
	PrintOpcode{ u8"fdiv.s", FormatType::dtf_s1f_s2f },
	PrintOpcode{ u8"fsqrt.s", FormatType::dtf_s1f },
	PrintOpcode{ u8"fsgnj.s", FormatType::dtf_s1f_s2f },
	PrintOpcode{ u8"fsgnjn.s", FormatType::dtf_s1f_s2f },
	PrintOpcode{ u8"fsgnjx.s", FormatType::dtf_s1f_s2f },
	PrintOpcode{ u8"fmin.s", FormatType::dtf_s1f_s2f },
	PrintOpcode{ u8"fmax.s", FormatType::dtf_s1f_s2f },
	PrintOpcode{ u8"fle.s", FormatType::dtf_s1f_s2f },
	PrintOpcode{ u8"flt.s", FormatType::dtf_s1f_s2f },
	PrintOpcode{ u8"feq.s", FormatType::dtf_s1f_s2f },
	PrintOpcode{ u8"fclass.s", FormatType::dti_s1f },

	PrintOpcode{ u8"fmadd.d", FormatType::dtf_s1f_s2f_s3f },
	PrintOpcode{ u8"fmsub.d", FormatType::dtf_s1f_s2f_s3f },
	PrintOpcode{ u8"fnmadd.d", FormatType::dtf_s1f_s2f_s3f },
	PrintOpcode{ u8"fnmsub.d", FormatType::dtf_s1f_s2f_s3f },
	PrintOpcode{ u8"fadd.d", FormatType::dtf_s1f_s2f },
	PrintOpcode{ u8"fsub.d", FormatType::dtf_s1f_s2f },
	PrintOpcode{ u8"fmul.d", FormatType::dtf_s1f_s2f },
	PrintOpcode{ u8"fdiv.d", FormatType::dtf_s1f_s2f },
	PrintOpcode{ u8"fsqrt.d", FormatType::dtf_s1f },
	PrintOpcode{ u8"fsgnj.d", FormatType::dtf_s1f_s2f },
	PrintOpcode{ u8"fsgnjn.d", FormatType::dtf_s1f_s2f },
	PrintOpcode{ u8"fsgnjx.d", FormatType::dtf_s1f_s2f },
	PrintOpcode{ u8"fmin.d", FormatType::dtf_s1f_s2f },
	PrintOpcode{ u8"fmax.d", FormatType::dtf_s1f_s2f },
	PrintOpcode{ u8"fle.d", FormatType::dtf_s1f_s2f },
	PrintOpcode{ u8"flt.d", FormatType::dtf_s1f_s2f },
	PrintOpcode{ u8"feq.d", FormatType::dtf_s1f_s2f },
	PrintOpcode{ u8"fclass.d", FormatType::dti_s1f },

	PrintOpcode{ u8"fcvt.w.s", FormatType::dti_s1f },
	PrintOpcode{ u8"fcvt.wu.s", FormatType::dti_s1f },
	PrintOpcode{ u8"fcvt.l.s", FormatType::dti_s1f },
	PrintOpcode{ u8"fcvt.lu.s", FormatType::dti_s1f },
	PrintOpcode{ u8"fcvt.w.d", FormatType::dti_s1f },
	PrintOpcode{ u8"fcvt.wu.d", FormatType::dti_s1f },
	PrintOpcode{ u8"fcvt.l.d", FormatType::dti_s1f },
	PrintOpcode{ u8"fcvt.lu.d", FormatType::dti_s1f },

	PrintOpcode{ u8"fcvt.s.w", FormatType::dtf_s1i },
	PrintOpcode{ u8"fcvt.s.wu", FormatType::dtf_s1i },
	PrintOpcode{ u8"fcvt.s.l", FormatType::dtf_s1i },
	PrintOpcode{ u8"fcvt.s.lu", FormatType::dtf_s1i },
	PrintOpcode{ u8"fcvt.d.w", FormatType::dtf_s1i },
	PrintOpcode{ u8"fcvt.d.wu", FormatType::dtf_s1i },
	PrintOpcode{ u8"fcvt.d.l", FormatType::dtf_s1i },
	PrintOpcode{ u8"fcvt.d.lu", FormatType::dtf_s1i },

	PrintOpcode{ u8"fmv.x.w", FormatType::dti_s1f },
	PrintOpcode{ u8"fmv.w.x", FormatType::dtf_s1i },
	PrintOpcode{ u8"fmv.x.d", FormatType::dti_s1f },
	PrintOpcode{ u8"fmv.d.x", FormatType::dtf_s1i },

	PrintOpcode{ u8"fcvt.d.s", FormatType::dtf_s1f },
	PrintOpcode{ u8"fcvt.s.d", FormatType::dtf_s1f },
};
static constexpr PrintOpcode pseudoStrings[] = {
	PrintOpcode{ u8"mv", FormatType::dti_s1i },
	PrintOpcode{ u8"not", FormatType::dti_s1i },
	PrintOpcode{ u8"neg", FormatType::dti_s2i },
	PrintOpcode{ u8"negw", FormatType::dti_s2i },
	PrintOpcode{ u8"sext.w", FormatType::dti_s1i },
	PrintOpcode{ u8"seqz", FormatType::dti_s1i },
	PrintOpcode{ u8"snez", FormatType::dti_s2i },
	PrintOpcode{ u8"sltz", FormatType::dti_s1i },
	PrintOpcode{ u8"sgtz", FormatType::dti_s2i },
	PrintOpcode{ u8"fmv.s", FormatType::dtf_s1f },
	PrintOpcode{ u8"fabs.s", FormatType::dtf_s1f },
	PrintOpcode{ u8"fneg.s", FormatType::dtf_s1f },
	PrintOpcode{ u8"fmv.d", FormatType::dtf_s1f },
	PrintOpcode{ u8"fabs.d", FormatType::dtf_s1f },
	PrintOpcode{ u8"fneg.d", FormatType::dtf_s1f },
	PrintOpcode{ u8"beqz", FormatType::s1i_jmp },
	PrintOpcode{ u8"bnez", FormatType::s1i_jmp },
	PrintOpcode{ u8"blez", FormatType::s2i_jmp },
	PrintOpcode{ u8"bgez", FormatType::s1i_jmp },
	PrintOpcode{ u8"bltz", FormatType::s1i_jmp },
	PrintOpcode{ u8"bgtz", FormatType::s2i_jmp },
	PrintOpcode{ u8"j", FormatType::imx },
	PrintOpcode{ u8"jal", FormatType::imx },
	PrintOpcode{ u8"jr", FormatType::s1i },
	PrintOpcode{ u8"jalr", FormatType::s1i },
	PrintOpcode{ u8"ret", FormatType::none },
	PrintOpcode{ u8"fence", FormatType::none },
	PrintOpcode{ u8"rdinstret", FormatType::dti },
	PrintOpcode{ u8"rdcycle", FormatType::dti },
	PrintOpcode{ u8"rdtime", FormatType::dti },
	PrintOpcode{ u8"csrr", FormatType::dti_csr },
	PrintOpcode{ u8"csrw", FormatType::csr_s1i },
	PrintOpcode{ u8"csrs", FormatType::csr_s1i },
	PrintOpcode{ u8"csrc", FormatType::csr_s1i },
	PrintOpcode{ u8"csrw", FormatType::csr_imx },
	PrintOpcode{ u8"csrs", FormatType::csr_imx },
	PrintOpcode{ u8"csrc", FormatType::csr_imx },
	PrintOpcode{ u8"frcsr", FormatType::dti },
	PrintOpcode{ u8"fscsr", FormatType::dti_s1i },
	PrintOpcode{ u8"fscsr", FormatType::s1i },
	PrintOpcode{ u8"frrm", FormatType::dti },
	PrintOpcode{ u8"fsrm", FormatType::dti_s1i },
	PrintOpcode{ u8"fsrm", FormatType::s1i },
	PrintOpcode{ u8"fsrmi", FormatType::dti_imx },
	PrintOpcode{ u8"fsrmi", FormatType::imx },
	PrintOpcode{ u8"fsflags", FormatType::dti },
	PrintOpcode{ u8"fsflags", FormatType::dti_s1i },
	PrintOpcode{ u8"fsflags", FormatType::s1i },
	PrintOpcode{ u8"fsflagsi", FormatType::dti_imx },
	PrintOpcode{ u8"fsflagsi", FormatType::imx },
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
	static_assert(sizeof(pseudoStrings) / sizeof(PrintOpcode) == size_t(rv64::Pseudo::_invalid), "string-table and pseudo-count must match");
	static_assert(sizeof(iRegisters) / sizeof(const char8_t*) == 32, "string-table must provide string for all 32 general-purpose registers");
	static_assert(sizeof(fRegisters) / sizeof(const char8_t*) == 32, "string-table must provide string for all 32 floating-point registers");

	/* check if the instruction is not valid, in which case format and other operands are irrelevant */
	if (inst.opcode == rv64::Opcode::_invalid)
		return u8"$invalid_instruction";
	std::u8string out;

	/* print the prefix */
	if (inst.size == 2)
		out = u8"c.";
	else if (inst.size > 4)
		out = u8"m.";

	/* print the opcode and fetch the format-type */
	const PrintOpcode* print = 0;
	if (inst.pseudo != rv64::Pseudo::_invalid)
		print = pseudoStrings + size_t(inst.pseudo);
	else
		print = opcodeStrings + size_t(inst.opcode);
	str::BuildTo(out, print->string);

	/* add the first parameter */
	switch (print->format) {
	case FormatType::fence:
		str::BuildTo(out,
			((inst.misc & 0b1000'0000) ? u8"i" : u8""), ((inst.misc & 0b0100'0000) ? u8"o" : u8""),
			((inst.misc & 0b0010'0000) ? u8"r" : u8""), ((inst.misc & 0b0001'0000) ? u8"w" : u8""),
			u8", ",
			((inst.misc & 0b0000'1000) ? u8"i" : u8""), ((inst.misc & 0b0000'0100) ? u8"o" : u8""),
			((inst.misc & 0b0000'0010) ? u8"r" : u8""), ((inst.misc & 0b0000'0001) ? u8"w" : u8""));
		break;
	case FormatType::csr_s1i:
	case FormatType::csr_imx:
		str::BuildTo(out, u8' ', str::As{ U"#05x", inst.misc });
		break;
	case FormatType::imx:
		str::BuildTo(out, u8' ', str::As{ U"#x", inst.imm });
		break;
	case FormatType::amo_dti_s1i:
	case FormatType::amo_dti_s1i_s2i:
		str::BuildTo(out, ((inst.misc & 0x02) != 0 ? u8".aq" : u8""), ((inst.misc & 0x01) != 0 ? u8".rl" : u8""));
		break;
	case FormatType::dti:
	case FormatType::dti_csr:
	case FormatType::dti_imx:
	case FormatType::dti_jmp:
	case FormatType::dti_jlr:
	case FormatType::dti_s1i:
	case FormatType::dti_s2i:
	case FormatType::dti_mem:
	case FormatType::dti_s1f:
	case FormatType::dti_imx_s1i:
	case FormatType::dti_s1i_imd:
	case FormatType::dti_s1i_s2i:
	case FormatType::dti_csr_s1i:
	case FormatType::dti_csr_imx:
		str::BuildTo(out, u8' ', iRegisters[inst.dest]);
		break;
	case FormatType::s1i:
	case FormatType::s1i_jmp:
	case FormatType::s1i_s2i_jmp:
		str::BuildTo(out, u8' ', iRegisters[inst.src1]);
		break;
	case FormatType::s2i_jmp:
	case FormatType::s2i_mem:
		str::BuildTo(out, u8' ', iRegisters[inst.src2]);
		break;
	case FormatType::s2f_mem:
		str::BuildTo(out, u8' ', fRegisters[inst.src2]);
		break;
	case FormatType::dtf_mem:
	case FormatType::dtf_s1f:
	case FormatType::dtf_s1i:
	case FormatType::dtf_imx_s1i:
	case FormatType::dtf_s1f_s2f:
	case FormatType::dtf_s1f_s2f_s3f:
		str::BuildTo(out, u8' ', fRegisters[inst.dest]);
		break;
	default:
		break;
	}

	/* add the second parameter */
	switch (print->format) {
	case FormatType::dti_csr:
	case FormatType::dti_csr_s1i:
	case FormatType::dti_csr_imx:
		str::BuildTo(out, u8", ", str::As{ U"#05x", inst.misc });
		break;
	case FormatType::csr_imx:
	case FormatType::dti_imx:
	case FormatType::dti_imx_s1i:
	case FormatType::dtf_imx_s1i:
		str::BuildTo(out, u8", ", str::As{ U"#x", inst.imm });
		break;
	case FormatType::dti_jmp:
	case FormatType::s1i_jmp:
	case FormatType::s2i_jmp:
		if (inst.imm < 0)
			str::BuildTo(out, u8", $(pc - ", -inst.imm, u8')');
		else
			str::BuildTo(out, u8", $(pc + ", inst.imm, u8')');
		break;
	case FormatType::dti_jlr:
		if (inst.imm < 0)
			str::BuildTo(out, u8", $(", iRegisters[inst.src1], u8" - ", -inst.imm, u8')');
		else
			str::BuildTo(out, u8", $(", iRegisters[inst.src1], u8" + ", inst.imm, u8')');
		break;
	case FormatType::csr_s1i:
	case FormatType::dti_s1i:
	case FormatType::dtf_s1i:
	case FormatType::dti_s1i_imd:
	case FormatType::dti_s1i_s2i:
		str::BuildTo(out, u8", ", iRegisters[inst.src1]);
		break;
	case FormatType::dti_s2i:
	case FormatType::s1i_s2i_jmp:
		str::BuildTo(out, u8", ", iRegisters[inst.src2]);
		break;
	case FormatType::dti_mem:
	case FormatType::dtf_mem:
	case FormatType::s2i_mem:
	case FormatType::s2f_mem:
		str::BuildTo(out, u8", ", inst.imm, u8'(', iRegisters[inst.src1], u8')');
		break;
	case FormatType::dti_s1f:
	case FormatType::dtf_s1f:
	case FormatType::dtf_s1f_s2f:
	case FormatType::dtf_s1f_s2f_s3f:
		str::BuildTo(out, u8", ", fRegisters[inst.src1]);
		break;
	case FormatType::amo_dti_s1i:
	case FormatType::amo_dti_s1i_s2i:
		str::BuildTo(out, u8", ", iRegisters[inst.dest]);
		break;
	default:
		break;
	}

	/* add the third parameter */
	switch (print->format) {
	case FormatType::dti_imx_s1i:
	case FormatType::dtf_imx_s1i:
	case FormatType::amo_dti_s1i:
	case FormatType::amo_dti_s1i_s2i:
	case FormatType::dti_csr_s1i:
		str::BuildTo(out, u8", ", iRegisters[inst.src1]);
		break;
	case FormatType::dtf_s1f_s2f:
	case FormatType::dtf_s1f_s2f_s3f:
		str::BuildTo(out, u8", ", fRegisters[inst.src2]);
		break;
	case FormatType::dti_csr_imx:
		str::BuildTo(out, u8", ", str::As{ U"#x", inst.imm });
		break;
	case FormatType::dti_s1i_imd:
		str::BuildTo(out, u8", ", inst.imm);
		break;
	case FormatType::dti_s1i_s2i:
		str::BuildTo(out, u8", ", iRegisters[inst.src2]);
		break;
	case FormatType::s1i_s2i_jmp:
		if (inst.imm < 0)
			str::BuildTo(out, u8", $(pc - ", -inst.imm, u8')');
		else
			str::BuildTo(out, u8", $(pc + ", inst.imm, u8')');
		break;
	default:
		break;
	}

	/* add the fourth parameter */
	switch (print->format) {
	case FormatType::amo_dti_s1i_s2i:
		str::BuildTo(out, u8", ", iRegisters[inst.src2]);
		break;
	case FormatType::dtf_s1f_s2f_s3f:
		str::BuildTo(out, u8", ", fRegisters[inst.src3]);
		break;
	default:
		break;
	}
	return out;
}
