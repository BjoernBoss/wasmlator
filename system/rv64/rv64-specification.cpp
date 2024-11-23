#include "rv64-specification.h"
#include "rv64-print.h"
#include "rv64-decoder.h"
#include "rv64-translation.h"

rv64::System::System() : env::System{ 0x400, 4, sizeof(rv64::Context) } {
	pTranslator = rv64::Translator::New();
}

std::unique_ptr<env::System> rv64::System::New(env::guest_t startup) {
	std::unique_ptr<rv64::System> spec = std::unique_ptr<rv64::System>{ new rv64::System{} };
	spec->pNextAddress = startup;
	return spec;
}

void rv64::System::setupCore(wasm::Module& mod) {
	gen::Core core{ mod };
}
std::vector<env::BlockExport> rv64::System::setupBlock(wasm::Module& mod) {
	gen::Block translator{ mod, pTranslator.get(), 1 };

	/* translate the next requested address */
	translator.run(pNextAddress);

	return translator.close();
}
void rv64::System::coreLoaded() {
	const uint8_t buffer[] = {
		/* addi a0, a0, 12 */
		0x13, 0x05, 0xc5, 0x00,

		/* jalr zero, 0(ra) => return */
		0x67, 0x80, 0x00, 0x00,
	};

	/* setup test-memory */
	env::Instance()->memory().mmap(0x0000, 0x8000, env::MemoryUsage::All);
	env::Instance()->memory().mwrite(pNextAddress, buffer, sizeof(buffer), 0);

	/* initialize the context */
	env::Instance()->context().get<rv64::Context>().pc = pNextAddress;

	/* request the translation of the first block */
	env::Instance()->startNewBlock();
}
void rv64::System::blockLoaded() {
	/* start execution of the next process-address */
	try {
		env::Instance()->mapping().execute(env::Instance()->context().get<rv64::Context>().pc);
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


std::unique_ptr<gen::Translator> rv64::Translator::New() {
	return std::unique_ptr<gen::Translator>{ new rv64::Translator{} };
}

void rv64::Translator::started(const gen::Writer& writer) {
	pDecoded.clear();
	pTempI64 = writer.sink().local(wasm::Type::i64, u8"temp_i64");
}
void rv64::Translator::completed(const gen::Writer& writer) {
	pDecoded.clear();
}
gen::Instruction  rv64::Translator::fetch(env::guest_t address) {
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
		/* check if it might be a call */
		if (inst.dest == reg::X1 || inst.dest == reg::X5)
			type = gen::InstType::primitive;
		else {
			type = gen::InstType::jumpDirect;
			target = address + inst.imm;
		}
		break;
	case rv64::Opcode::jump_and_link_reg:
		/* check if it might be a call */
		if ((inst.dest == reg::X1 || inst.dest == reg::X5) && ((inst.src1 != reg::X1 && inst.src1 != reg::X5) || inst.src1 == inst.dest))
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
void rv64::Translator::produce(const gen::Writer& writer, const gen::Instruction* data, size_t count) {
	for (size_t i = 0; i < count; ++i)
		rv64::Translate(data[i], pDecoded[data[i].data], writer, pTempI64);
}
