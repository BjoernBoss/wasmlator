#pragma once

#include "../sys-common.h"

namespace sys::detail {
	namespace consts {
		/* used by prlimit64 */
		static constexpr uint32_t rLimitCpu = 0;
		static constexpr uint32_t rLimitFSize = 1;
		static constexpr uint32_t rLimitData = 2;
		static constexpr uint32_t rLimitStack = 3;
		static constexpr uint32_t rLimitCore = 4;
		static constexpr uint32_t rLimitRSS = 5;
		static constexpr uint32_t rLimitNumProc = 6;
		static constexpr uint32_t rLimitNumFile = 7;
		static constexpr uint32_t rLimitMemLock = 8;
		static constexpr uint32_t rLimitAddressSpace = 9;
		static constexpr uint32_t rLimitEnd = 10;
		static constexpr uint64_t rLimitInfinity = uint64_t(-1);

		/* used by getrandom */
		static constexpr uint32_t randomNonBlock = 0x01;
		static constexpr uint32_t randomRandom = 0x02;

		static constexpr uint32_t randomMask = consts::randomNonBlock | consts::randomRandom;
	}

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
		int64_t set_robust_list(env::guest_t head, uint64_t size) const;
		int64_t prlimit64(uint64_t pid, uint64_t res, env::guest_t new_rlim, env::guest_t old_rlim) const;
		int64_t getrandom(env::guest_t buf, uint64_t buflen, uint32_t flags) const;
	};
}
