#pragma once

#include "../sys-common.h"
#include "sys-primitive-debugger.h"
#include "sys-primitive-execcontext.h"

namespace sys {
	class Primitive;

	/* primitive single-threaded system, which set up an environment, loads a static-elf
	*	file to be executed, and passes the calls to the cpu implementation, as well as
	*	a system-v ABI conform initial stack configuration
	*	Note: The assumption is made, that the stack grows downwards */
	class Primitive final : public env::System {
		friend class detail::PrimitiveExecContext;
		friend class detail::PrimitiveSyscall;
		friend class detail::PrimitiveDebugger;
	private:
		static constexpr env::guest_t StartOfStackAlignment = 128;
		static constexpr env::guest_t StackSize = 0x40'0000;
		static constexpr uint32_t TranslationDepth = 3;
		static constexpr uint32_t PageSize = 0x1000;

	private:
		std::vector<std::u8string> pArgs;
		std::vector<std::u8string> pEnvs;
		detail::PrimitiveExecContext* pExecContext = 0;
		sys::Cpu* pCpu = 0;
		env::guest_t pAddress = 0;
		bool pDebug = false;

	public:
		Primitive() = default;
		Primitive(sys::Primitive&&) = delete;
		Primitive(const sys::Primitive&) = delete;
		~Primitive() = default;

	private:
		env::guest_t fPrepareStack() const;
		void fExecute();

	public:
		static bool Create(std::unique_ptr<sys::Cpu>&& cpu, const std::vector<std::u8string>& args, const std::vector<std::u8string>& envs, bool debug, bool logBlocks, bool traceBlocks);

	public:
		bool setupCore(wasm::Module& mod) final;
		bool coreLoaded() final;
		std::vector<env::BlockExport> setupBlock(wasm::Module& mod) final;
		void blockLoaded() final;
		void shutdown() final;
	};
}
