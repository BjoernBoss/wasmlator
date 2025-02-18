#pragma once

#include "rv64-common.h"
#include "rv64-translation.h"
#include "rv64-print.h"
#include "rv64-decoder.h"
#include "rv64-pseudo.h"

namespace rv64 {
	class Cpu;

	/* riscv 64-bit */
	class Cpu final : public sys::Cpu {
	private:
		std::vector<rv64::Instruction> pDecoded;
		rv64::Translate pTranslator;
		sys::Writer* pWriter = 0;

	private:
		Cpu();

	private:
		rv64::Instruction fFetchRaw(env::guest_t address) const;
		rv64::Instruction fFetch(env::guest_t address) const;

	private:
		uint64_t fHandleHWProbe(uint64_t pairs, uint64_t pairCount, uint64_t cpuCount, uint64_t cpus, uint64_t flags) const;

	public:
		static std::unique_ptr<sys::Cpu> New();

	public:
		bool setupCpu(sys::Writer* writer) final;
		bool setupCore(wasm::Module& mod) final;
		bool setupContext(env::guest_t pcAddress, env::guest_t spAddress) final;

	public:
		void started(env::guest_t address) final;
		void completed() final;
		gen::Instruction fetch(env::guest_t address) final;
		void produce(env::guest_t address, const uintptr_t* self, size_t count) final;

	public:
		sys::SyscallArgs syscallGetArgs() const final;
		void syscallSetResult(uint64_t value) final;
		std::u8string getExceptionText(uint64_t id) const final;

	public:
		std::vector<std::u8string> debugQueryNames() const final;
		std::pair<std::u8string, uint8_t> debugDecode(env::guest_t address) const final;
		uint64_t debugGetValue(size_t index) const final;
		void debugSetValue(size_t index, uint64_t value) final;
	};
}
