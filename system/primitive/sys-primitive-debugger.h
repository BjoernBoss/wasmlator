#pragma once

#include "../debugger/sys-debugger.h"

namespace sys {
	class Primitive;

	namespace detail {
		class PrimitiveDebugger final : public sys::Debuggable {
		private:
			sys::Primitive* pPrimitive = 0;

		private:
			PrimitiveDebugger(sys::Primitive* primitive);

		public:
			static std::unique_ptr<detail::PrimitiveDebugger> New(sys::Primitive* primitive);

		public:
			std::unique_ptr<sys::CpuDebuggable> getCpu() final;
			env::guest_t getPC() const final;
			void setPC(env::guest_t pc) final;
			void run() final;
		};
	}
}
