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
		struct {
			env::guest_t address = 0;
			env::guest_t next = 0;
			int64_t result = 0;
			bool inplace = false;
		} pCurrent;

	public:
		Syscall() = default;

	private:
		std::u8string fReadString(env::guest_t address) const;

	private:
		void fWrap(std::function<int64_t()> callback);
		int64_t fDispatch();
		int64_t fHandleIds(std::u8string_view name) const;
		int64_t fHandleUName(env::guest_t addr) const;

	public:
		bool setup(sys::Userspace* userspace, env::guest_t endOfData, std::u8string_view wDirectory);
		void handle(env::guest_t address, env::guest_t nextAddress);

	public:
		detail::FileIO& fileIO();
		void callIncomplete();
		void callContinue(std::function<int64_t()> callback);
	};
}
