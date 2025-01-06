#pragma once

#include "../sys-common.h"

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
}
