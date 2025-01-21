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

	/* assumptions about structures and enums made by the userspace-environment */
	namespace linux {
		enum class AuxiliaryType : uint32_t {
			null = 0,
			ignore = 1,
			executableFd = 2,
			phAddress = 3,
			phEntrySize = 4,
			phCount = 5,
			pageSize = 6,
			baseInterpreter = 7,
			flags = 8,
			entry = 9,
			notELF = 10,
			uid = 11,
			euid = 12,
			gid = 13,
			egid = 14,
			clockTick = 17,
			secure = 23,
			random = 25,
			executableFilename = 31
		};
		enum class FileMode : uint32_t {
			socket = 0xc000,
			link = 0xa000,
			regular = 0x8000,
			blockDevice = 0x6000,
			directory = 0x4000,
			charDevice = 0x2000,
			fifoPipe = 0x1000,
		};

		struct UName {
			char8_t sysName[65] = { 0 };
			char8_t nodeName[65] = { 0 };
			char8_t release[65] = { 0 };
			char8_t version[65] = { 0 };
			char8_t machine[65] = { 0 };
		};
		struct FileStats {
			uint64_t dev = 0;
			uint64_t inode = 0;
			uint32_t mode = 0;
			uint32_t nlinks = 0;
			uint32_t uid = 0;
			uint32_t gid = 0;
			uint32_t _unused0 = 0;
			uint64_t rdev = 0;
			uint64_t size = 0;
			uint32_t blockSize = 0;
			uint32_t _unused1 = 0;
			uint64_t blockCount = 0;
			uint64_t atime_sec = 0;
			uint64_t atime_ns = 0;
			uint64_t mtime_sec = 0;
			uint64_t mtime_ns = 0;
			uint64_t ctime_sec = 0;
			uint64_t ctime_ns = 0;
		};
	}

	enum class ArchType : uint8_t {
		unknown,
		x86_64,
		riscv64
	};

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
		fstatat,
		exit_group,
		access,
		faccessat,
		faccessat2
	};

	struct SyscallArgs {
		uint64_t args[6] = { 0 };
		uint64_t rawIndex = 0;
		sys::SyscallIndex index = sys::SyscallIndex::unknown;
	};
}
