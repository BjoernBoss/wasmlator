#pragma once

#include "../environment/environment.h"
#include "../generate/generate.h"

struct NullSystem final : public env::System {
	NullSystem() {}
	void setupCore(wasm::Module& mod) override {}
	std::vector<env::BlockExport> setupBlock(wasm::Module& mod) override { return {}; }
	void coreLoaded() override {}
	void blockLoaded() override {}
};

struct NullTranslator final : public gen::Translator {
	NullTranslator() = default;
	void started(const gen::Writer& writer, env::guest_t address) override {}
	void completed(const gen::Writer& writer) override {}
	gen::Instruction fetch(env::guest_t address) override { return {}; }
	void produce(const gen::Writer& writer, env::guest_t address, const uintptr_t* self, size_t count) override {}
};
