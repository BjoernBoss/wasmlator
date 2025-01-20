#pragma once

#include <ustring/ustring.h>
#include <cinttypes>
#include <type_traits>
#include <vector>
#include <string>
#include <memory>
#include <utility>
#include <unordered_set>
#include <algorithm>
#include <limits>
#include <map>

#include "../environment/environment.h"
#include "../generate/generate.h"
#include "../host/host-interact.h"
#include "../util/util-logger.h"
#include "../util/util-unique.h"
#include "../util/util-path.h"

namespace sys {
	class Userspace;
	namespace detail {
		class Syscall;
	}

	namespace detail {
		namespace errCode {
			static constexpr int64_t eSuccess = 0;
			static constexpr int64_t eNoEntry = -2;
			static constexpr int64_t eIO = -5;
			static constexpr int64_t eBadFd = -9;
			static constexpr int64_t eNoMemory = -12;
			static constexpr int64_t eAccess = -13;
			static constexpr int64_t eFault = -14;
			static constexpr int64_t eExists = -17;
			static constexpr int64_t eNotDirectory = -20;
			static constexpr int64_t eIsDirectory = -21;
			static constexpr int64_t eInvalid = -22;
			static constexpr int64_t eReadOnly = -30;
			static constexpr int64_t eLoop = -40;

			/* custom error (not mapped to any linux errors) */
			static constexpr int64_t eUnknown = -0xffff;
		}

		struct FlushInstCache : public env::Exception {
		public:
			FlushInstCache(env::guest_t address) : env::Exception{ address } {}
		};

		struct CpuException : public env::Exception {
		public:
			uint64_t id = 0;

		public:
			CpuException(env::guest_t address, uint64_t id) : env::Exception{ address }, id{ id } {}
		};

		struct UnknownSyscall : public env::Exception {
		public:
			uint64_t index = 0;

		public:
			UnknownSyscall(env::guest_t address, uint64_t index) : env::Exception{ address }, index{ index } {}
		};

		struct AwaitingSyscall : public env::Exception {
		public:
			AwaitingSyscall(env::guest_t address) : env::Exception{ address } {}
		};
	}

	enum class SyscallIndex : uint32_t {
		unknown,
		brk,
		getuid,
		geteuid,
		getgid,
		getegid,
		uname,
		openat,
		open,
		read,
		readv,
		write,
		writev,
		mmap,
		readlinkat,
		readlink,
		fstat,
		exit_group
	};

	struct SyscallArgs {
		uint64_t args[6] = { 0 };
		uint64_t rawIndex = 0;
		sys::SyscallIndex index = sys::SyscallIndex::unknown;
	};
}
