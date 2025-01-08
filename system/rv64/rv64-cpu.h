#pragma once

#include "rv64-common.h"
#include "rv64-translation.h"
#include "rv64-print.h"
#include "rv64-decoder.h"

namespace rv64 {
	class Cpu;

	namespace detail {
		class CpuSyscall final : public sys::CpuSyscallable {
		private:
			CpuSyscall() = default;

		public:
			static std::unique_ptr<detail::CpuSyscall> New();

		public:
			sys::SyscallArgs getArgs() const final;
			void setResult(uint64_t value) final;
		};

		class CpuDebug final : public sys::CpuDebuggable {
		private:
			rv64::Cpu* pCpu = 0;

		private:
			CpuDebug(rv64::Cpu* cpu);

		public:
			static std::unique_ptr<detail::CpuDebug> New(rv64::Cpu* cpu);

		public:
			std::vector<std::u8string> queryNames() const final;
			std::pair<std::u8string, uint8_t> decode(env::guest_t address) const final;
			uint64_t getValue(size_t index) const final;
			void setValue(size_t index, uint64_t value) final;
		};
	}

	/* riscv 64-bit */
	class Cpu final : public sys::Cpu {
		friend class detail::CpuSyscall;
		friend class detail::CpuDebug;
	private:
		std::vector<rv64::Instruction> pDecoded;
		std::unique_ptr<sys::ExecContext> pContext;
		rv64::Translate pTranslator;

	private:
		Cpu();

	private:
		rv64::Instruction fFetch(env::guest_t address) const;

	public:
		static std::unique_ptr<sys::Cpu> New();

	public:
		bool setupCpu(std::unique_ptr<sys::ExecContext>&& execContext) final;
		bool setupCore(wasm::Module& mod) final;
		bool setupContext(env::guest_t pcAddress, env::guest_t spAddress) final;
		std::u8string getExceptionText(uint64_t id) const final;

	public:
		void started(env::guest_t address) final;
		void completed() final;
		gen::Instruction fetch(env::guest_t address) final;
		void produce(env::guest_t address, const uintptr_t* self, size_t count) final;

	public:
		std::unique_ptr<sys::CpuSyscallable> getSyscall() final;
		std::unique_ptr<sys::CpuDebuggable> getDebug() final;
	};
}
