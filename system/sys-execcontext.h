#pragma once

#include "../environment/environment.h"
#include "../generate/generate.h"

namespace sys {
	class ExecContext {
	private:
		bool pMultiThreaded = false;
		bool pUserspace = false;

	protected:
		constexpr ExecContext(bool multiThreaded, bool userspace) : pMultiThreaded{ multiThreaded }, pUserspace{ userspace } {}

	public:
		virtual ~ExecContext() = default;

	public:
		/* generate the code to perform a syscall (must not be called from non-userspace applications)
		*	Note: may abort the control-flow */
		virtual void syscall(env::guest_t address, env::guest_t nextAddress) = 0;

		/* generate the code to throw an exception (must not be called from non-userspace applications)
		*	Note: will abort the control-flow */
		virtual void throwException(uint64_t id, env::guest_t address, env::guest_t nextAddress) = 0;

		/* generate the code to flush the memory cache
		*	Note: may abort the control-flow */
		virtual void flushMemCache(env::guest_t address, env::guest_t nextAddress) = 0;

		/* generate the code to flush the instruction cache
		*	Note: will abort the control-flow */
		virtual void flushInstCache(env::guest_t address, env::guest_t nextAddress) = 0;

	public:
		constexpr bool multiThreaded() const {
			return pMultiThreaded;
		}
		constexpr bool userspace() const {
			return pUserspace;
		}
	};
}
