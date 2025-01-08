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
			static std::unique_ptr<detail::PrimitiveSyscall> New(sys::Primitive* primitive);

		public:
			std::unique_ptr<sys::CpuSyscallable> getCpu() final;
			void run(env::guest_t address) final;
		};
	}
}
