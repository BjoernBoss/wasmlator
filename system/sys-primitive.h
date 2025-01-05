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
			void syscall(env::guest_t address, env::guest_t nextAddress) final;
			void throwException(uint64_t id, env::guest_t address, env::guest_t nextAddress) final;
			void flushMemCache(env::guest_t address, env::guest_t nextAddress) final;
			void flushInstCache(env::guest_t address, env::guest_t nextAddress) final;
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
		static constexpr env::guest_t StartOfStackAlignment = 128;
		static constexpr env::guest_t StackSize = 0x40'0000;
		static constexpr uint32_t TranslationDepth = 4;
		static constexpr uint32_t PageSize = 0x1000;

	private:
		std::vector<std::u8string> pArgs;
		std::vector<std::u8string> pEnvs;
		sys::Cpu* pCpu = 0;
		env::guest_t pAddress = 0;
		struct {
			uint32_t flushInst = 0;
			uint32_t syscall = 0;
			uint32_t exception = 0;
		} pRegistered;

	public:
		Primitive() = default;
		Primitive(sys::Primitive&&) = delete;
		Primitive(const sys::Primitive&) = delete;
		~Primitive() = default;

	private:
		env::guest_t fPrepareStack() const;

	public:
		static bool Create(std::unique_ptr<sys::Cpu>&& cpu, const std::vector<std::u8string>& args, const std::vector<std::u8string>& envs, bool debug);

	public:
		bool setupCore(wasm::Module& mod) final;
		bool coreLoaded() final;
		std::vector<env::BlockExport> setupBlock(wasm::Module& mod) final;
		void blockLoaded() final;
		void shutdown() final;
	};
}
