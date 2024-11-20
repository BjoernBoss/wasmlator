#pragma once

#include "../environment/environment.h"
#include "../generate/generate.h"

struct NullSystem final : public env::System {
	NullSystem() : env::System{ 0x1000, 2, 2 } {}
	void setupCore(wasm::Module& mod) override {}
	std::vector<env::BlockExport> setupBlock(wasm::Module& mod) override { return {}; }
	void coreLoaded() override {}
	void blockLoaded() override {}
};

struct NullTranslator final : public gen::Translator {
	NullTranslator() : gen::Translator{ 2 } {}
	void started(const gen::Writer& writer) override {}
	void completed(const gen::Writer& writer) override {}
	gen::Instruction fetch(env::guest_t address) override { return {}; }
	void produce(const gen::Writer& writer, const gen::Instruction* data, size_t count) override {}
};
