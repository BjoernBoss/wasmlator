#include "interface.h"

#include <wasgen/wasm.h>
#include <stdlib.h>

#include "../interface/host.h"
#include "../environment/env-process.h"
#include "../generate/translator/gen-translator.h"
#include "../generate/core/gen-core.h"

struct CPUContext {
	uint32_t eax = 0;
};

struct Test {};

struct TestCPUImpl final : public sys::Specification {
private:
	uint32_t pTestIndex = 0;

public:
	TestCPUImpl() : sys::Specification{ 4, 4, sizeof(CPUContext), false } {}

private:
	void fTestBlock(gen::Translator& trans) {
		trans.run(0x1234);
		trans.run(0x5678);
	}

public:
	void setupCore(wasm::Module& mod) override {
		pTestIndex = env::Instance()->interact().defineCallback([]() {
			host::Log(u8"test-callback called!");
			throw Test{};
			});

		gen::Core _core{ mod };

		wasm::Sink sink{ mod.function(u8"test", { wasm::Type::i64 }, { wasm::Type::i64 }, wasm::Export{}) };
		sink[wasm::inst::Param::Get(0)];
		sink[wasm::inst::U64::Const(1)];
		sink[wasm::inst::U64::Add()];
	}
	std::vector<env::BlockExport> setupBlock(wasm::Module& mod) override {
		gen::Translator _translator{ mod };

		fTestBlock(_translator);

		return _translator.close();
	}
	void coreLoaded() override {
		host::Log(u8"Test: ", env::Instance()->interact().call(u8"test", 63));

		writer::TextWriter _writer;
		{
			wasm::Module _module{ &_writer };
			gen::Translator _translator{ _module };
			fTestBlock(_translator);
		}
		host::Log(_writer.output());

		env::Instance()->startNewBlock();
	}
	void blockLoaded() override {
		try {
			auto res = env::Instance()->mapping().execute(0x1234);
			host::Log(u8"Execution of 0x1234 completed: [", size_t(res), u8']');
		}
		catch (const Test& t) {
			host::Log(u8"Test caught!");
		}
	}

public:
	void translationStarted() override {}
	void translationCompleted() override {}
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
			return gen::Instruction{ 3, address, 0, 2, gen::InstType::endOfBlock };

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
				writer.invokeVoid(pTestIndex);
				writer.call(0x5678, data[i]);
				break;
			case 2:
				writer.sink()[wasm::inst::U32::Const(1)];
				writer.sink()[wasm::inst::Branch::If(*writer.hasTarget(data[i].address))];
				break;
			case 3:
				writer.sink()[wasm::inst::U64::Const(0x9abc)];
				writer.ret();
				break;
			}
		}
	}
};

void main_startup() {
	host::Log(u8"Main: Application startup entered");
	env::Process::Create(std::make_unique<TestCPUImpl>());
	host::Log(u8"Main: Application startup exited");

	env::Instance()->release();
	env::Process::Create(std::make_unique<TestCPUImpl>());
}
