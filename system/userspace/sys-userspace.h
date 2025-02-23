#pragma once

#include "../sys-common.h"

#include "../sys-cpu.h"
#include "../debugger/sys-debugger.h"
#include "../syscall/sys-syscall.h"
#include "../writer/sys-writer.h"
#include "../elf/sys-elf.h"

namespace sys {
	static constexpr uint32_t DefTranslationDepth = 2;

	namespace detail {
		static constexpr env::guest_t StartOfStackAlignment = 128;
		static constexpr env::guest_t StackSize = 0x80'0000;
		static constexpr uint32_t PageSize = 0x1000;
		static constexpr uint32_t MaxProcessCount = 1;
		static constexpr const char8_t* ResolveLocations[] = {
			u8"", u8"/", u8"/bin/", u8"/lib/"
		};
	}

	struct RunConfig {
		std::vector<std::u8string> args;
		std::vector<std::u8string> envs;
		std::u8string binary;
		uint32_t translationDepth = sys::DefTranslationDepth;
		gen::TraceType trace = gen::TraceType::none;
		bool logBlocks = false;
	};

	/* userspace single-threaded system, which set up an environment, loads an elf
	*	file to be executed, and passes the calls to the cpu implementation, as well as
	*	a system-v ABI conform initial stack configuration (only 64 bit support)
	*	Note: The assumption is made, that the stack grows downwards
	*	Note: The pc is managed by the userspace object */
	class Userspace final : public env::System {
	private:
		std::vector<std::u8string> pArgs;
		std::vector<std::u8string> pEnvs;
		std::u8string pBinaryPath;
		std::u8string pBinaryActual;
		elf::LoadState pLoaded;
		detail::Syscall pSyscall;
		sys::Debugger pDebugger;
		sys::Writer pWriter;
		sys::Cpu* pCpu = 0;
		env::guest_t pAddress = 0;
		size_t pResolveIndex = 0;

	private:
		Userspace() = default;
		Userspace(sys::Userspace&&) = delete;
		Userspace(const sys::Userspace&) = delete;

	private:
		bool fSetup(std::unique_ptr<sys::Userspace>&& system, std::unique_ptr<sys::Cpu>&& cpu, const sys::RunConfig& config, bool debug);
		std::u8string_view fArchType(sys::ArchType architecture) const;
		env::guest_t fPrepareStack() const;
		void fStartLoad(const std::u8string& path);
		bool fBinaryLoaded(const std::u8string& actual, const uint8_t* data, size_t size);
		bool fLoadCompleted();

	private:
		void fCheckContinue() const;
		void fExecute();

	public:
		static bool Create(std::unique_ptr<sys::Cpu>&& cpu, const sys::RunConfig& config);
		static sys::Debugger* Debug(std::unique_ptr<sys::Cpu>&& cpu, const sys::RunConfig& config);

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
		void checkContinue();
	};
}
