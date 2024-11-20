#pragma once

#include "../environment/environment.h"
#include "../generate/generate.h"

#include "rv64-common.h"
#include "rv64-decoder.h"

namespace rv64 {
	/* riscv 64-bit */
	class System final : public env::System {
	private:
		env::guest_t pNextAddress = 0;
		std::vector<rv64::Instruction> pDecoded;
		std::unique_ptr<gen::Translator> pTranslator;

	private:
		System();

	public:
		static std::unique_ptr<env::System> New(env::guest_t startup);

	public:
		void setupCore(wasm::Module& mod) final;
		std::vector<env::BlockExport> setupBlock(wasm::Module& mod) final;
		void coreLoaded() final;
		void blockLoaded() final;
	};

	class Translator final : public gen::Translator {
	private:
		std::vector<rv64::Instruction> pDecoded;

	private:
		Translator();

	public:
		static std::unique_ptr<gen::Translator> New();

	public:
		void started(const gen::Writer& writer) override;
		void completed(const gen::Writer& writer) override;
		gen::Instruction fetch(env::guest_t address) override;
		void produce(const gen::Writer& writer, const gen::Instruction* data, size_t count) override;
	};
}
