#pragma once

#include "../syscall/sys-syscall.h"

namespace sys {
	class Primitive;

	namespace detail {
		class PrimitiveSyscall final : public sys::Syscallable {
		private:
			sys::Primitive* pPrimitive = 0;

		private:
			PrimitiveSyscall(sys::Primitive* primitive);

		public:
			static std::unique_ptr<sys::detail::PrimitiveSyscall> New(sys::Primitive* primitive);

		public:
			sys::SyscallArgs getArgs() const final;
			void setResult(uint64_t value) final;
			void run(env::guest_t address) final;
		};
	}
}
