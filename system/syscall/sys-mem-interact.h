#pragma once

#include "../sys-common.h"

namespace sys::detail {
	static constexpr uint64_t MaxLockedMemory = 0;

	namespace consts {
		/* used by mmap */
		static constexpr uint32_t mmProtNone = 0x00;
		static constexpr uint32_t mmProtRead = 0x01;
		static constexpr uint32_t mmProtWrite = 0x02;
		static constexpr uint32_t mmProtExec = 0x04;
		static constexpr uint32_t mmProtMask = consts::mmProtNone | consts::mmProtRead |
			consts::mmProtWrite | consts::mmProtExec;

		/* used by mmap (deny-write/exeuctable are ignored) */
		static constexpr uint32_t mmFlagShared = 0x000001;
		static constexpr uint32_t mmFlagPrivate = 0x000002;
		static constexpr uint32_t mmFlagSharedValidate = 0x000003;
		static constexpr uint32_t mmFlagFixed = 0x000010;
		static constexpr uint32_t mmFlagAnonymous = 0x000020;
		static constexpr uint32_t mmFlagSync = 0x080000;
		static constexpr uint32_t mmFlagFixedNoReplace = 0x100000;
		static constexpr uint32_t mmFlagDenyWrite = 0x000800;
		static constexpr uint32_t mmFlagExecutable = 0x001000;
		static constexpr uint32_t mmFlagMask = consts::mmFlagShared | consts::mmFlagPrivate |
			consts::mmFlagSharedValidate | consts::mmFlagFixed | consts::mmFlagAnonymous |
			consts::mmFlagShared | consts::mmFlagSync | consts::mmFlagFixedNoReplace |
			consts::mmFlagDenyWrite | consts::mmFlagExecutable;
	}

	struct MemShared {
		uint64_t length = 0;
	};

	class MemoryInteract {
	private:
		struct {
			env::guest_t init = 0;
			env::guest_t current = 0;
			env::guest_t aligned = 0;
		} pBrk;
		std::map<env::guest_t, detail::MemShared> pShared;
		env::guest_t pPageSize = 0;
		detail::Syscall* pSyscall = 0;

	public:
		MemoryInteract() = default;

	private:
		env::guest_t fPageOffset(env::guest_t address) const;
		env::guest_t fPageAlignUp(env::guest_t address) const;
		void fRemoveShared(env::guest_t address, uint64_t length);
		bool fCheckRange(env::guest_t address, uint64_t length, bool replace);
		int64_t fMapRange(env::guest_t address, uint64_t length, uint32_t usage, uint32_t flags);

	public:
		bool setup(detail::Syscall* syscall, env::guest_t endOfData);
		int64_t brk(env::guest_t address);
		int64_t mmap(env::guest_t address, uint64_t length, uint32_t protect, uint32_t flags, int64_t fd, uint64_t offset);
		int64_t mprotect(env::guest_t address, uint64_t length, uint32_t protect);
		int64_t munmap(env::guest_t address, uint64_t length);
	};
}
