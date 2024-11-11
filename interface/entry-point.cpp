#include "interface.h"

#include <wasgen/wasm.h>
#include <stdlib.h>

#include "../interface/host.h"
#include "../environment/env-process.h"
#include "../generate/gen-translator.h"
#include "../generate/core/gen-core.h"

struct CPUContext {
	uint32_t eax = 0;
};

struct TestCPUImpl final : public sys::Specification {
public:
	TestCPUImpl() : sys::Specification{ 4, 4, sizeof(CPUContext), false } {}

public:
	void setupCore(wasm::Module& mod) override {
		gen::SetupCore(mod);
	}
	void setupBlock(wasm::Module& mod) override {
	}
	void coreLoaded() override {
		writer::TextWriter _writer;
		wasm::Module _module{ &_writer };

		gen::Translator _translator{ _module };

		_translator.run(0x1234);
		_translator.run(0x5678);

		std::vector<env::BlockExport> exported = _translator.close();
		_module.close();

		host::Log(_writer.output());
	}
	void bodyLoaded() override {}

public:
	void blockStarted() override {}
	void blockCompleted() override {}
	gen::Instruction fetchInstruction(env::guest_t address) override {
		if (address == 0x1234)
			return gen::Instruction{ 1, address, 0, 2, gen::InstType::primitive };
		if (address == 0x1236)
			return gen::Instruction{ 2, address, 0x123a, 1, gen::InstType::conditionalDirect };
		if (address == 0x1237)
			return gen::Instruction{ 1, address, 0, 3, gen::InstType::primitive };
		if (address == 0x123a)
			return gen::Instruction{ 0, address, 0, 2, gen::InstType::endOfBlock };

		if (address == 0x5678)
			return gen::Instruction{ 0, address, 0, 2, gen::InstType::endOfBlock };

		return gen::Instruction{ 0, 0, 0, 0, gen::InstType::invalid };
	}
	void produceInstructions(const gen::Writer& writer, const gen::Instruction* data, size_t count) override {
		for (size_t i = 0; i < count; ++i) {
			switch (data[i].data) {
			case 0:
				writer.sink()[wasm::inst::U64::Const(0x5678)];
				writer.ret();
				break;
			case 1:
				writer.sink()[wasm::inst::Nop()];
				break;
			case 2:
				writer.sink()[wasm::inst::U32::Const(1)];
				writer.sink()[wasm::inst::Branch::If(*writer.hasTarget(data[i].address))];
				break;
			}
		}
	}
};

void main_startup() {
	host::Log(u8"Main: Application startup entered");
	env::Process::Create(std::make_unique<TestCPUImpl>());
	host::Log(u8"Main: Application startup exited");
}
