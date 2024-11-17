#pragma once

#include "../sys-specification.h"
#include "rv64-common.h"
#include "rv64-decoder.h"

namespace rv64 {
	/* riscv 64-bit */
	class Specification final : public sys::Specification {
	private:
		env::guest_t pNextAddress = 0;
		std::vector<rv64::Instruction> pDecoded;

	private:
		Specification();

	public:
		static std::unique_ptr<sys::Specification> New(env::guest_t startup);

	public:
		void setupCore(wasm::Module& mod) final;
		std::vector<env::BlockExport> setupBlock(wasm::Module& mod) final;
		void coreLoaded() final;
		void blockLoaded() final;

	public:
		void translationStarted() final;
		void translationCompleted() final;
		gen::Instruction fetchInstruction(env::guest_t address) final;
		void produceInstructions(const gen::Writer& writer, const gen::Instruction* data, size_t count) final;
	};
}
