#pragma once

#include "../system/sys-specification.h"

struct NullSpecification final : public sys::Specification {
public:
	NullSpecification() : sys::Specification{ 1, 1, 1, false } {}

public:
	void setupCore(wasm::Module& mod) override {}
	void setupBlock(wasm::Module& mod) override {}
	void coreLoaded() override {}
	void bodyLoaded() override {}

public:
	void blockStarted() override {}
	void blockCompleted() override {}
	gen::Instruction fetchInstruction(env::guest_t address) override {
		return gen::Instruction{ 0, 0, 0, 0, gen::InstType::invalid };
	}
	void produceInstructions(const gen::Writer& writer, const gen::Instruction* data, size_t count) override {}
};
