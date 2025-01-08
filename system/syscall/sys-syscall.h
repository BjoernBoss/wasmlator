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

	private:
		Syscall() = default;
		Syscall(sys::Syscall&&) = delete;
		Syscall(const sys::Syscall&) = delete;

	private:
		void fHandle();

	public:
		static std::unique_ptr<sys::Syscall> New(std::unique_ptr<sys::Syscallable> provider);

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
