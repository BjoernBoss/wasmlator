#include "rv64-cpu.h"

static host::Logger logger{ u8"rv64::cpu" };

rv64::Cpu::Cpu() : sys::Cpu{ rv64::MemoryCaches, sizeof(rv64::Context) } {}

std::unique_ptr<sys::Cpu> rv64::Cpu::New() {
	return std::unique_ptr<rv64::Cpu>{ new rv64::Cpu{} };
}

bool rv64::Cpu::setupCpu(std::unique_ptr<sys::ExecContext>&& execContext) {
	pContext = std::move(execContext);

	/* validate the context is currently supported */
	if (!pContext->userspace()) {
		logger.error(u8"Currently only support for userspace execution");
		return false;
	}
	if (pContext->multiThreaded()) {
		logger.error(u8"Currently no support for multi-threaded execution");
		return false;
	}
	return true;
}
bool rv64::Cpu::setupCore(wasm::Module& mod) {
	return true;
}
bool rv64::Cpu::setupContext(env::guest_t pcAddress, env::guest_t spAddress) {
	rv64::Context& ctx = env::Instance()->context().get<rv64::Context>();

	/* no need to write the pc to the context, as all exceptions will implicitly
	*	already know the exact pc but write the stack-pointer to the sp-register,
	*	the rest can remain as-is (will implicitly be null) */
	ctx.sp = spAddress;

	return true;
}
std::u8string rv64::Cpu::getExceptionText(uint64_t id) const {
	switch (id) {
	case rv64::Translate::EBreakException:
		return u8"ebreak";
	case rv64::Translate::MisAlignedException:
		return u8"misaligned";
	default:
		return u8"%unknown%";
	}
}
void rv64::Cpu::started(env::guest_t address) {
	pDecoded.clear();
	pTranslator.resetAll(pContext.get());
}
void rv64::Cpu::completed() {
	pDecoded.clear();
	pTranslator.resetAll(0);
}
gen::Instruction rv64::Cpu::fetch(env::guest_t address) {
	/* check that the address is 2-byte aligned */
	if ((address & 0x01) != 0) {
		rv64::Instruction& inst = (pDecoded.emplace_back() = rv64::Instruction{});
		inst.opcode = rv64::Opcode::misaligned;
		inst.size = 1;
		return gen::Instruction{ gen::InstType::endOfBlock, inst.size, 0 };
	}

	/* decode the next instruction (first as compressed, and afterwards as
	*	large, in order to prevent potential memory-alignment issues) */
	uint32_t code = env::Instance()->memory().code<uint16_t>(address);
	rv64::Instruction& inst = (pDecoded.emplace_back() = rv64::Decode16(code));
	if (inst.opcode == rv64::Opcode::_invalid) {
		code |= (env::Instance()->memory().code<uint16_t>(address + 2) << 16);
		inst = rv64::Decode32(code);
	}
	logger.fmtTrace(u8"RV64: {:#018x}:{:#010x} {}", address, code, rv64::ToString(inst));

	/* translate the instruction to the generated instruction-type */
	gen::InstType type = gen::InstType::primitive;
	env::guest_t target = 0;
	switch (inst.opcode) {
	case rv64::Opcode::_invalid:
		type = gen::InstType::invalid;
		break;
	case rv64::Opcode::branch_eq:
	case rv64::Opcode::branch_ne:
	case rv64::Opcode::branch_lt_s:
	case rv64::Opcode::branch_ge_s:
	case rv64::Opcode::branch_lt_u:
	case rv64::Opcode::branch_ge_u:
		type = gen::InstType::conditionalDirect;
		target = address + inst.imm;
		break;
	case rv64::Opcode::jump_and_link_imm:
		/* check if it might be a call and otherwise consider it a direct jump */
		if (inst.isCall())
			type = gen::InstType::primitive;
		else {
			type = gen::InstType::jumpDirect;
			target = address + inst.imm;
		}
		break;
	case rv64::Opcode::jump_and_link_reg:
		/* check if it might be a call and otherwise consider it a return or indirect jump */
		if (inst.isCall())
			type = gen::InstType::primitive;
		else
			type = gen::InstType::endOfBlock;
		break;
	default:
		break;
	}

	/* check if the target is considered mis-aligned, in which case it does not need to be translated */
	if (inst.isMisAligned(address))
		type = gen::InstType::endOfBlock;

	/* construct the output-instruction format */
	return gen::Instruction{ type, target, inst.size, pDecoded.size() - 1 };
}
void rv64::Cpu::produce(env::guest_t address, const uintptr_t* self, size_t count) {
	pTranslator.start(address);
	for (size_t i = 0; i < count; ++i)
		pTranslator.next(pDecoded[self[i]]);
}
