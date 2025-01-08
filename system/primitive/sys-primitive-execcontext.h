#pragma once

#include "../sys-common.h"
#include "../syscall/sys-syscall.h"

namespace sys {
	class Primitive;

	namespace detail {
		class PrimitiveExecContext final : public sys::ExecContext {
		private:
			sys::Primitive* pPrimitive = 0;
			std::unique_ptr<sys::Syscall> pSyscall;
			struct {
				uint64_t id = 0;
				env::guest_t address = 0;
			} pException;
			struct {
				uint32_t flushInst = 0;
				uint32_t exception = 0;
			} pRegistered;

		private:
			PrimitiveExecContext(sys::Primitive* primitive);

		public:
			static std::unique_ptr<detail::PrimitiveExecContext> New(sys::Primitive* primitive);

		public:
			void syscall(env::guest_t address, env::guest_t nextAddress) final;
			void throwException(uint64_t id, env::guest_t address, env::guest_t nextAddress) final;
			void flushMemCache(env::guest_t address, env::guest_t nextAddress) final;
			void flushInstCache(env::guest_t address, env::guest_t nextAddress) final;

		public:
			bool coreLoaded();
		};

		struct FlushInstCache : public env::Exception {
		public:
			FlushInstCache(env::guest_t address) : env::Exception{ address } {}
		};

		struct CpuException : public env::Exception {
		public:
			uint64_t id = 0;

		public:
			CpuException(env::guest_t address, uint64_t id) : env::Exception{ address }, id{ id } {}
		};
	}
}
