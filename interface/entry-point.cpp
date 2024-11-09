#include "interface.h"

#include <wasgen/wasm.h>
#include <stdlib.h>

#include "../interface/host.h"
#include "../env/env-process.h"
#include "../trans/trans-translator.h"
#include "../trans/core/trans-core.h"

struct CPUContext {
	uint32_t eax = 0;
};

struct TransInterface final : public trans::TranslationInterface {
	void blockStarted() override {}
	void blockCompleted() override {}

	trans::Instruction fetch(env::guest_t addr) override {
		if (addr == 0x1234)
			return trans::Instruction{ 1, addr, 0, 2, trans::InstType::primitive };
		if (addr == 0x1236)
			return trans::Instruction{ 2, addr, 0x123a, 1, trans::InstType::conditionalDirect };
		if (addr == 0x1237)
			return trans::Instruction{ 1, addr, 0, 3, trans::InstType::primitive };
		if (addr == 0x123a)
			return trans::Instruction{ 0, addr, 0, 2, trans::InstType::endOfBlock };

		if (addr == 0x5678)
			return trans::Instruction{ 0, addr, 0, 2, trans::InstType::endOfBlock };

		return trans::Instruction{ 0, 0, 0, 0, trans::InstType::invalid };
	}
	void produce(const trans::Writer& writer, const trans::Instruction* data, size_t count) override {
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
	trans::Translator _translator{ _module, &_interface, 4 };

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
	trans::SetupCore(_module);

	/* upload the core */
	const std::vector<uint8_t>& data = _writer.output();
	env::Instance()->loadCore(data.data(), data.size(), []() {

		translate(0x1234);
		});

	host::Log(u8"Main: Application startup exited");
}
