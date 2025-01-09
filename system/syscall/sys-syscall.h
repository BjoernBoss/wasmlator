#pragma once

#include "sys-syscallable.h"

namespace sys {
	/* requires env::Process to be setup and the core to be loaded
	*	Note: returns nullptr on construction failure */
	class Syscall {
	private:
		std::unique_ptr<sys::Syscallable> pProvider;
		std::unique_ptr<sys::CpuSyscallable> pCpu;
		struct {
			env::guest_t address = 0;
			env::guest_t next = 0;
		} pActive;
		uint32_t pSyscallHandlerId = 0;
		struct {
			env::guest_t init = 0;
			env::guest_t current = 0;
			env::guest_t aligned = 0;
		} pBrk;
		env::guest_t pPageSize = 0;

	private:
		Syscall() = default;
		Syscall(sys::Syscall&&) = delete;
		Syscall(const sys::Syscall&) = delete;

	private:
		std::u8string fReadString(env::guest_t address) const;

	private:
		void fHandle();
		uint64_t fHandleIds(std::u8string_view name) const;
		env::guest_t fHandleBrk(env::guest_t addr);
		uint64_t fHandleUName(env::guest_t addr) const;
		uint64_t fHandleOpenAt(int64_t dirfd, env::guest_t pathname, uint64_t flags, uint64_t mode) const;

	public:
		static std::unique_ptr<sys::Syscall> New(std::unique_ptr<sys::Syscallable> provider, env::guest_t endOfData);

	public:
		void makeSyscall(env::guest_t address, env::guest_t nextAddress);
	};

	/* thrown whenever an unknown syscall id is encountered */
	struct UnknownSyscall : public env::Exception {
	public:
		uint64_t index = 0;

	public:
		UnknownSyscall(env::guest_t address, uint64_t index) : env::Exception{ address }, index{ index } {}
	};
}
