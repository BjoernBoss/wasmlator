#pragma once

#include <vector>
#include <string>

#include "../environment/environment.h"
#include "../generate/generate.h"

#include "sys-cpu.h"
#include "sys-execcontext.h"

namespace sys {
	class Primitive;

	namespace detail {
		class PrimitiveExecContext final : public sys::ExecContext {
		private:
			sys::Primitive* pPrimitive = 0;

		private:
			PrimitiveExecContext(sys::Primitive* primitive);

		public:
			static std::unique_ptr<sys::ExecContext> New(sys::Primitive* primitive);

		public:
			void syscall(const gen::Writer& writer, env::guest_t address, env::guest_t nextAddress) final;
			void throwException(uint64_t id, const gen::Writer& writer, env::guest_t address, env::guest_t nextAddress) final;
			void flushMemCache(const gen::Writer& writer, env::guest_t address, env::guest_t nextAddress) final;
			void flushInstCache(const gen::Writer& writer, env::guest_t address, env::guest_t nextAddress) final;
		};

		struct FlushInstCache : public env::Exception {
		public:
			FlushInstCache(env::guest_t address) : env::Exception{ address } {}
		};
	}

	/* primitive single-threaded system, which set up an environment, loads a static-elf
	*	file to be executed, and passes the calls to the cpu implementation, as well as
	*	a system-v ABI conform initial stack configuration
	*	Note: The assumption is made, that the stack grows downwards */
	class Primitive final : public env::System {
		friend class detail::PrimitiveExecContext;
	private:
		static constexpr env::guest_t InitialStackAddress = 0x7800'0000'0000;
		static constexpr env::guest_t StartOfStackAlignment = 128;
		static constexpr env::guest_t StackSize = 0x40'0000;
		static constexpr uint32_t PageSize = 0x1000;

	private:
		std::vector<std::u8string> pArgs;
		std::vector<std::u8string> pEnvs;
		std::unique_ptr<sys::Cpu> pCpu;
		env::guest_t pAddress = 0;
		struct {
			uint32_t flushInst = 0;
			uint32_t syscall = 0;
			uint32_t exception = 0;
		} pRegistered;

	private:
		Primitive(uint32_t memoryCaches, uint32_t contextSize);

	private:
		env::guest_t fPrepareStack() const;

	public:
		static std::unique_ptr<env::System> New(std::unique_ptr<sys::Cpu>&& cpu, const std::vector<std::u8string>& args, const std::vector<std::u8string>& envs);

	public:
		void setupCore(wasm::Module& mod) final;
		std::vector<env::BlockExport> setupBlock(wasm::Module& mod) final;
		void coreLoaded() final;
		void blockLoaded() final;
	};
}
