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
			static std::unique_ptr<sys::Debuggable> New(sys::Primitive* primitive);

		public:
			void getState(std::unordered_map<std::u8string, uintptr_t>& state) const final;
			env::guest_t getPC() const final;
			std::u8string decode(uintptr_t address) const final;
			void setValue(std::u8string_view name, uintptr_t value) final;
			void setPC(env::guest_t pc) final;
			void run() final;
		};
	}
}
