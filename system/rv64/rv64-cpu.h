#pragma once

#include "rv64-common.h"
#include "rv64-translation.h"
#include "rv64-print.h"
#include "rv64-decoder.h"

namespace rv64 {
	/* riscv 64-bit */
	class Cpu final : public sys::Cpu {
	private:
		std::vector<rv64::Instruction> pDecoded;
		std::unique_ptr<sys::ExecContext> pContext;
		rv64::Translate pTranslator;

	private:
		Cpu();

	public:
		static std::unique_ptr<sys::Cpu> New();

	public:
		void setupCpu(std::unique_ptr<sys::ExecContext>&& execContext) final;
		void setupCore(wasm::Module& mod) final;
		void setupContext(env::guest_t pcAddress, env::guest_t spAddress) final;
		std::u8string getExceptionText(uint64_t id) const final;
		void started(env::guest_t address) final;
		void completed() final;
		gen::Instruction fetch(env::guest_t address) final;
		void produce(env::guest_t address, const uintptr_t* self, size_t count) final;
	};
}
