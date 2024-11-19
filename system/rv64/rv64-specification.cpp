#include "rv64-specification.h"

#include "rv64-print.h"
#include "../../environment/env-process.h"
#include "../../generate/core/gen-core.h"
#include "../../generate/translator/gen-translator.h"

rv64::Specification::Specification() : sys::Specification{ 4, 4, sizeof(rv64::Context) }{}

std::unique_ptr<sys::Specification> rv64::Specification::New(env::guest_t startup) {
	std::unique_ptr<rv64::Specification> spec = std::unique_ptr<rv64::Specification>{ new rv64::Specification{} };
	spec->pNextAddress = startup;
	return spec;
}

void rv64::Specification::setupCore(wasm::Module& mod) {
	gen::Core core{ mod };
}
std::vector<env::BlockExport> rv64::Specification::setupBlock(wasm::Module& mod) {
	gen::Translator translator{ mod };

	/* translate the next requested address */
	translator.run(pNextAddress);

	return translator.close();
}
void rv64::Specification::coreLoaded() {
	const uint8_t buffer[] = {
		/* addi a0, a0, 12 */
		0x13, 0x05, 0xc5, 0x00,

		/* jalr zero, 0(ra) => return */
		0x67, 0x80, 0x00, 0x00,
	};

	/* setup test-memory */
	env::Instance()->memory().mmap(0x0000, 8 * env::PageSize, env::MemoryUsage::All);
	env::Instance()->memory().mwrite(pNextAddress, buffer, sizeof(buffer), 0);

	/* initialize the context */
	rv64::Context context{};
	context.pc = pNextAddress;
	env::Instance()->context().write(context);

	/* request the translation of the first block */
	env::Instance()->startNewBlock();
}
void rv64::Specification::blockLoaded() {
	/* start execution of the next process-address */
	try {
		env::Instance()->mapping().execute(env::Instance()->context().read<rv64::Context>().pc);
	}
	catch (const env::Translate& e) {
		host::Debug(str::u8::Format(u8"Translate caught: {:#018x}", e.address));
		pNextAddress = e.address;
		env::Instance()->startNewBlock();
	}
	catch (const env::NotDecodable& e) {
		host::Debug(str::u8::Format(u8"NotDecodable caught: {:#018x}", e.address));
	}
}

void rv64::Specification::translationStarted() {
	pDecoded.clear();
}
void rv64::Specification::translationCompleted() {
	pDecoded.clear();
}
gen::Instruction rv64::Specification::fetchInstruction(env::guest_t address) {
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
		target = address + inst.immediate;
		break;
	case rv64::Opcode::jump_and_link_imm:
		type = gen::InstType::jumpDirect;
		target = address + inst.immediate;
		break;
	case rv64::Opcode::jump_and_link_reg:
		type = gen::InstType::endOfBlock;
		break;
	default:
		break;
	}

	/* construct the output-instruction format */
	return gen::Instruction{ type, target, 4, pDecoded.size() - 1 };
}
void rv64::Specification::produceInstructions(const gen::Writer& writer, const gen::Instruction* data, size_t count) {}
