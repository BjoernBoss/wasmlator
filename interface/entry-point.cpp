#include "interface.h"

#include <wasgen/wasm.h>
#include <stdlib.h>

#include "../interface/host.h"
#include "../env/env-process.h"
#include "../gen/gen-translator.h"
#include "../gen/core/gen-core.h"

struct CPUContext {
	uint32_t eax = 0;
};

struct TransInterface final : public gen::TranslationInterface {
	void blockStarted() override {}
	void blockCompleted() override {}

	gen::Instruction fetch(env::guest_t address) override {
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
	void produce(const gen::Writer& writer, const gen::Instruction* data, size_t count) override {
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

static void translate(env::guest_t address) {
	writer::TextWriter _writer;
	wasm::Module _module{ &_writer };

	TransInterface _interface;
	gen::Translator _translator{ _module, &_interface, 4 };

	_translator.run(0x1234);
	_translator.run(0x5678);

	std::vector<env::BlockExport> exported = _translator.close();
	_module.close();

	host::Log(_writer.output());
}

void main_startup() {
	host::Log(u8"Main: Application startup entered");
	env::Process::Create(4, sizeof(CPUContext));

	/* setup the core */
	writer::BinaryWriter _writer;
	wasm::Module _module{ &_writer };
	gen::SetupCore(_module);
	_module.close();

	/* upload the core */
	const std::vector<uint8_t>& data = _writer.output();
	env::Instance()->loadCore(data.data(), data.size(), []() {

		translate(0x1234);
		});

	host::Log(u8"Main: Application startup exited");
}
