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
			static std::unique_ptr<sys::detail::PrimitiveDebugger> New(sys::Primitive* primitive);

		public:
			env::guest_t getPC() const final;
			void setPC(env::guest_t pc) final;
			void run() final;
			std::vector<std::u8string> queryNames() const final;
			std::pair<std::u8string, uint8_t> decode(uintptr_t address) const final;
			uintptr_t getValue(size_t index) const final;
			void setValue(size_t index, uintptr_t value) final;
		};
	}
}
