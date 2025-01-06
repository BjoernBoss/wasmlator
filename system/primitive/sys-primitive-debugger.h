#pragma once

#include "../sys-common.h"

namespace sys {
	class Primitive;

	namespace detail {
		class PrimitiveDebugger final : public sys::Debuggable {
		private:
			sys::Primitive* pPrimitive = 0;

		private:
			PrimitiveDebugger(sys::Primitive* primitive);

		public:
			static std::unique_ptr<sys::detail::PrimitiveDebugger> New(sys::Primitive* primitive);

		public:
			sys::Cpu* getCpu() const final;
			env::guest_t getPC() const final;
			void setPC(env::guest_t pc) final;
			void run() final;
		};
	}
}
