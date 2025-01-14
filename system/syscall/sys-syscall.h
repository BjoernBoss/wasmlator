#pragma once

#include "../sys-common.h"

#include "sys-file-io.h"
#include "sys-mem-interact.h"

namespace sys::detail {
	class Syscall {
	private:
		sys::Userspace* pUserspace = 0;
		detail::FileIO pFileIO;
		detail::MemoryInteract pMemory;

	public:
		Syscall() = default;

	private:
		std::u8string fReadString(env::guest_t address) const;

	private:
		void fHandle(env::guest_t address);
		int64_t fWrapped(env::guest_t address, const sys::SyscallArgs& args);
		int64_t fHandleIds(std::u8string_view name) const;
		int64_t fHandleUName(env::guest_t addr) const;

	public:
		bool setup(sys::Userspace* userspace, env::guest_t endOfData, std::u8string_view currentDirectory);
		void handle(env::guest_t address, env::guest_t nextAddress);
	};
}
