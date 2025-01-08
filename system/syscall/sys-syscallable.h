#pragma once

#include "../sys-common.h"

namespace sys {
	enum class SyscallIndex : uint32_t {
		unknown,
		read,
		write,
		brk,
		getuid,
		geteuid,
		getgid,
		getegid
	};

	struct SyscallArgs {
		uint64_t args[6] = { 0 };
		uint64_t rawIndex = 0;
		sys::SyscallIndex index = sys::SyscallIndex::unknown;
	};

	class Syscallable {
	protected:
		Syscallable() = default;

	public:
		virtual ~Syscallable() = default;

	public:
		/* fetch the arguments for a unix syscall
		*	Note: will only be called within code produced by Syscall::makeSyscall */
		virtual sys::SyscallArgs getArgs() const = 0;

		/* set the result of the last syscall being performed
		*	Note: will always be called after fetching the syscall-arguments, before resuming the execution */
		virtual void setResult(uint64_t value) = 0;

		/* continue execution after a blocking syscall has completed (address is the next address after the syscall) */
		virtual void run(env::guest_t address) = 0;
	};
}
