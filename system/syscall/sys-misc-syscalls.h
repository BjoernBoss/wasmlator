#pragma once

#include "../sys-common.h"

namespace sys::detail {
	class MiscSyscalls {
	private:
		detail::Syscall* pSyscall = 0;

	public:
		MiscSyscalls() = default;

	public:
		bool setup(detail::Syscall* syscall);
		int64_t getuid() const;
		int64_t geteuid() const;
		int64_t getgid() const;
		int64_t getegid() const;
		int64_t uname(env::guest_t address) const;
		int64_t gettimeofday(env::guest_t tv, env::guest_t tz) const;
		int64_t set_tid_address(env::guest_t tidptr) const;
	};
}
