#include "rv64-cpu.h"
#include "rv64-print.h"
#include "rv64-decoder.h"
#include "rv64-translation.h"

rv64::Cpu::Cpu() : sys::Cpu{ 1, sizeof(rv64::Context) } {}

std::unique_ptr<sys::Cpu> rv64::Cpu::New() {
	return std::unique_ptr<rv64::Cpu>{ new rv64::Cpu{} };
}

void rv64::Cpu::setupCore(wasm::Module& mod) {}
void rv64::Cpu::setupContext(env::guest_t address) {}
void rv64::Cpu::started(const gen::Writer& writer) {
	pDecoded.clear();
}
void rv64::Cpu::completed(const gen::Writer& writer) {
	pDecoded.clear();
}
gen::Instruction  rv64::Cpu::fetch(env::guest_t address) {
	/* check that the address is 4-byte aligned */
	if ((address & 0x03) != 0)
		return gen::Instruction{};

	/* decode the next instruction */
	const rv64::Instruction& inst = pDecoded.emplace_back() = rv64::Decode(env::Instance()->memory().code<uint32_t>(address));
	host::Debug(str::u8::Format(u8"RV64: {:#018x} {}", address, rv64::ToString(inst)));

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

	/* construct the output-instruction format */
	return gen::Instruction{ type, target, 4, pDecoded.size() - 1 };
}
void rv64::Cpu::produce(const gen::Writer& writer, const gen::Instruction* data, size_t count) {
	for (size_t i = 0; i < count; ++i)
		rv64::Translate(data[i], pDecoded[data[i].data], writer);
}
