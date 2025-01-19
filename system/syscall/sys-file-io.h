#pragma once

#include "../sys-common.h"
#include "file-nodes/sys-root-nodes.h"

namespace sys::detail {
	static constexpr size_t MaxFollowSymLinks = 16;

	namespace fileFlags {
		/* ignored: O_DIRECT, O_LARGEFILE, O_NOCTTY, O_SYNC, FASYNC, O_NOATIME */
		static constexpr uint32_t readOnly = 0x000000;
		static constexpr uint32_t writeOnly = 0x000001;
		static constexpr uint32_t readWrite = 0x000002;
		static constexpr uint32_t create = 0x000040;
		static constexpr uint32_t exclusive = 0x000080;
		static constexpr uint32_t truncate = 0x000200;
		static constexpr uint32_t directory = 0x010000;
		static constexpr uint32_t noSymlinkFollow = 0x020000;
		static constexpr uint32_t closeOnExecute = 0x080000;
		static constexpr uint32_t openOnly = 0x200000;
		static constexpr uint32_t mask = 0x2b0ec3;
	}
	namespace fileMode {
		static constexpr uint32_t rOwner = 0x0100;
		static constexpr uint32_t wOwner = 0x0080;
		static constexpr uint32_t xOwner = 0x0040;
		static constexpr uint32_t rGroup = 0x0020;
		static constexpr uint32_t wGroup = 0x0010;
		static constexpr uint32_t xGroup = 0x0008;
		static constexpr uint32_t rOther = 0x0004;
		static constexpr uint32_t wOther = 0x0002;
		static constexpr uint32_t xOther = 0x0001;
		static constexpr uint32_t mask = 0x01ff;
	}
	static constexpr int fdWDirectory = -100;

	class FileIO {
	private:
		struct Instance {
			detail::SharedNode node;
			uint64_t id = 0;
			size_t user = 0;
			bool directory = false;
		};
		struct Open {
			size_t instance = 0;
			bool used = false;
			bool read = false;
			bool write = false;
			bool modify = false;
			bool closeOnExecute = false;
		};

	private:
		detail::SharedNode pRoot;
		std::vector<Instance> pInstance;
		std::vector<Open> pOpen;
		std::vector<uint8_t> pBuffer;
		std::vector<uint64_t> pCached;
		detail::Syscall* pSyscall = 0;
		size_t pLinkFollow = 0;

	public:
		FileIO() = default;

	private:
		bool fCheckFd(int64_t fd) const;
		int64_t fCheckRead(int64_t fd) const;
		int64_t fCheckWrite(int64_t fd) const;
		bool fCheckAccess(const env::FileStats* stats, bool read, bool write, bool execute) const;
		int64_t fCheckPath(int64_t dirfd, std::u8string_view path, std::u8string& actual);

	private:
		int64_t fResolveNode(const std::u8string& path, bool follow, bool exact, std::function<int64_t(int64_t, const std::u8string&, detail::SharedNode, const env::FileStats*, bool)> callback);
		int64_t fResolveNext(const std::u8string& path, std::u8string_view lookup, bool follow, bool exact, detail::SharedNode node, const env::FileStats* stats, std::function<int64_t(int64_t, const std::u8string&, detail::SharedNode, const env::FileStats*, bool)> callback);
		void fDropInstance(size_t instance);

	private:
		int64_t fOpenAt(int64_t dirfd, std::u8string_view path, uint64_t flags, uint64_t mode);
		int64_t fSetupFile(detail::SharedNode node, bool directory, bool read, bool write, bool modify, bool closeOnExecute);

	private:
		int64_t fRead(size_t instance, std::function<int64_t(int64_t)> callback);
		int64_t fWrite(size_t instance) const;
		int64_t fReadLinkAt(int64_t dirfd, std::u8string_view path, env::guest_t address, uint64_t size);

	public:
		bool setup(detail::Syscall* syscall);

	public:
		int64_t openat(int64_t dirfd, std::u8string_view path, uint64_t flags, uint64_t mode);
		int64_t open(std::u8string_view path, uint64_t flags, uint64_t mode);
		int64_t read(int64_t fd, env::guest_t address, uint64_t size);
		int64_t readv(int64_t fd, env::guest_t vec, uint64_t count);
		int64_t write(int64_t fd, env::guest_t address, uint64_t size);
		int64_t writev(int64_t fd, env::guest_t vec, uint64_t count);
		int64_t readlinkat(int64_t dirfd, std::u8string_view path, env::guest_t address, uint64_t size);
		int64_t readlink(std::u8string_view path, env::guest_t address, uint64_t size);
		int64_t fstat(int64_t fd, env::guest_t address);
	};
}
