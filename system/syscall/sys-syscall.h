#pragma once

#include "../sys-common.h"

#include "sys-file-io.h"

namespace sys::detail {
	class Syscall {
	private:
		sys::Userspace* pUserspace = 0;
		detail::FileIO pFileIO;
		struct {
			env::guest_t init = 0;
			env::guest_t current = 0;
			env::guest_t aligned = 0;
		} pBrk;
		env::guest_t pPageSize = 0;

	public:
		Syscall() = default;

	private:
		std::u8string fReadString(env::guest_t address) const;

	private:
		void fHandle(env::guest_t address);
		uint64_t fWrapped(env::guest_t address, const sys::SyscallArgs& args);
		uint64_t fHandleIds(std::u8string_view name) const;
		env::guest_t fHandleBrk(env::guest_t addr);
		uint64_t fHandleUName(env::guest_t addr) const;
		uint64_t fHandleOpenAt(int64_t dirfd, env::guest_t pathname, uint64_t flags, uint64_t mode);

	public:
		bool setup(sys::Userspace* userspace, env::guest_t endOfData, std::u8string_view currentDirectory);
		void handle(env::guest_t address, env::guest_t nextAddress);
	};
}
