#include "interface.h"

#include <wasgen/wasm.h>
#include <stdlib.h>

#include "../interface/host.h"
#include "../env/env-process.h"
#include "../trans/trans-translator.h"

struct CPUContext {
	uint32_t eax = 0;
};

struct TransInterface final : public trans::TranslationInterface {
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

	}
};

void main_startup() {
	host::Log(u8"Main: Application startup entered");
	env::Process::Create(4, sizeof(CPUContext));

	writer::BinaryWriter _writer;
	wasm::Module _module{ &_writer };
	TransInterface _interface;

	trans::Translator _translator{ true, _module, &_interface, 4 };
	_translator.run(0x1234);
	_translator.run(0x5678);

	std::vector<env::BlockExport> exported = _translator.close();
	_module.close();
	const std::vector<uint8_t>& data = _writer.output();

	env::Instance()->loadCore(data.data(), data.size(), exported, []() {
		env::Instance()->mapping().execute(0x1234);
		});

	host::Log(u8"Main: Application startup exited");
}
