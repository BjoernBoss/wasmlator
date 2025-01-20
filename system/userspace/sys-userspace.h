#pragma once

#include "../sys-common.h"

#include "../sys-cpu.h"
#include "../debugger/sys-debugger.h"
#include "../syscall/sys-syscall.h"
#include "../writer/sys-writer.h"
#include "../elf/sys-elf.h"

namespace sys {
	/* userspace single-threaded system, which set up an environment, loads a static-elf
	*	file to be executed, and passes the calls to the cpu implementation, as well as
	*	a system-v ABI conform initial stack configuration (only 64 bit support)
	*	Note: The assumption is made, that the stack grows downwards
	*	Note: The pc is managed by the userspace object */
	class Userspace final : public env::System {
	private:
		static constexpr env::guest_t StartOfStackAlignment = 128;
		static constexpr env::guest_t StackSize = 0x40'0000;
		static constexpr uint32_t TranslationDepth = 3;
		static constexpr uint32_t PageSize = 0x1000;

	private:
		std::vector<std::u8string> pArgs;
		std::vector<std::u8string> pEnvs;
		std::u8string pBinary;
		std::u8string pInterpreter;
		detail::Syscall pSyscall;
		sys::Debugger pDebugger;
		sys::Writer pWriter;
		sys::Cpu* pCpu = 0;
		env::guest_t pAddress = 0;

	private:
		Userspace() = default;
		Userspace(sys::Userspace&&) = delete;
		Userspace(const sys::Userspace&) = delete;

	private:
		std::u8string_view fArchType(sys::ArchType architecture) const;
		env::guest_t fPrepareStack(const sys::ElfLoaded& loaded, bool is64Bit) const;
		bool fBinaryLoaded(const uint8_t* data, size_t size);
		void fExecute();
		void fStartLoad(const std::u8string& path);

	public:
		static bool Create(std::unique_ptr<sys::Cpu>&& cpu, const std::u8string& binary, const std::vector<std::u8string>& args, const std::vector<std::u8string>& envs, bool logBlocks, bool traceBlocks, sys::Debugger** debugger);

	public:
		bool setupCore(wasm::Module& mod) final;
		void coreLoaded() final;
		std::vector<env::BlockExport> setupBlock(wasm::Module& mod) final;
		void blockLoaded() final;
		void shutdown() final;

	public:
		const sys::Cpu* cpu() const;
		sys::Cpu* cpu();
		env::guest_t getPC() const;
		void setPC(env::guest_t address);
		void execute();
	};
}
