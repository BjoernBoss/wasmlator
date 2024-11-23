#pragma once

#include "../../environment/environment.h"
#include "../../generate/generate.h"
#include "../sys-cpu.h"

#include "rv64-common.h"

namespace rv64 {
	/* riscv 64-bit */
	class Cpu final : public sys::Cpu {
	private:
		std::vector<rv64::Instruction> pDecoded;
		wasm::Variable pTempI64;

	private:
		Cpu();

	public:
		static std::unique_ptr<sys::Cpu> New();

	public:
		void setupCore(wasm::Module& mod) override;
		void setupContext(env::guest_t address) override;
		void started(const gen::Writer& writer) override;
		void completed(const gen::Writer& writer) override;
		gen::Instruction fetch(env::guest_t address) override;
		void produce(const gen::Writer& writer, const gen::Instruction* data, size_t count) override;
	};
}
