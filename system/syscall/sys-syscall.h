#pragma once

#include "../sys-common.h"

#include "sys-file-io.h"
#include "sys-mem-interact.h"

namespace sys::detail {
	namespace fs {
		static constexpr uint32_t ThisUesr = 1001;
		static constexpr uint32_t ThisGroup = 1001;
		static constexpr uint32_t DefOwner = fs::ThisUesr;
		static constexpr uint32_t DefGroup = fs::ThisGroup;
		static constexpr uint32_t RootOwner = 0;
		static constexpr uint32_t RootGroup = 0;

		static constexpr uint16_t OwnerAllElseRW = env::FileOwner(true, true, true) | env::FileGroup(true, false, true) | env::FileOther(true, false, true);
		static constexpr uint16_t All = env::FileOwner(true, true, true) | env::FileGroup(true, true, true) | env::FileOther(true, true, true);
		static constexpr uint16_t ReadWrite = env::FileOwner(true, true, false) | env::FileGroup(true, true, false) | env::FileOther(true, true, false);
		static constexpr uint16_t ReadOnly = env::FileOwner(true, true, false) | env::FileGroup(true, true, false) | env::FileOther(true, true, false);
		static constexpr uint16_t ReadExecute = env::FileOwner(true, false, true) | env::FileGroup(true, false, true) | env::FileOther(true, false, true);
	}

	struct ProcessConfig {
		std::u8string userName;
		std::u8string path;
		std::u8string wDirectory;
		uint32_t uid = fs::ThisUesr;
		uint32_t gid = fs::ThisGroup;
		uint32_t euid = fs::ThisUesr;
		uint32_t egid = fs::ThisGroup;
		uint32_t pid = 1;
		uint32_t pgid = 1;
	};

	class Syscall {
	private:
		sys::Userspace* pUserspace = 0;
		detail::FileIO pFileIO;
		detail::MemoryInteract pMemory;
		struct {
			env::guest_t address = 0;
			env::guest_t next = 0;
			size_t nested = 0;
			int64_t result = 0;
			bool completed = false;
		} pCurrent;
		detail::ProcessConfig pConfig;
		std::u8string pMachine;

	public:
		Syscall() = default;

	private:
		std::u8string fReadString(env::guest_t address) const;

	private:
		void fWrap(bool inplace, std::function<int64_t()> callback);
		int64_t fDispatch();
		int64_t fHandleUName(env::guest_t addr) const;

	public:
		bool setup(sys::Userspace* userspace, env::guest_t endOfData, std::u8string_view path, std::u8string_view machine);
		void handle(env::guest_t address, env::guest_t nextAddress);

	public:
		detail::ProcessConfig& process();
		detail::FileIO& files();
		int64_t callIncomplete();
		void callContinue(std::function<int64_t()> callback);
	};
}
