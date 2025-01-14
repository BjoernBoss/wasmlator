#pragma once

#include "../sys-common.h"
#include "sys-file-nodes.h"

namespace sys::detail {
	namespace fileFlags {
		/* ignored: O_DIRECT, O_LARGEFILE, O_NOCTTY, O_SYNC, FASYNC, O_NOATIME */
		static constexpr uint32_t readOnly = 0x000000;
		static constexpr uint32_t writeOnly = 0x000001;
		static constexpr uint32_t readWrite = 0x000002;
		static constexpr uint32_t create = 0x000040;
		static constexpr uint32_t exclusive = 0x000080;
		static constexpr uint32_t truncate = 0x000200;
		static constexpr uint32_t append = 0x000400;
		static constexpr uint32_t nonBlocking = 0x000800;
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

	/* Note: implements separate link-following mechanic, as symlinks might
	*	exist, which are not part of the actual raw underlying filesystem */
	class FileIO {
	private:
		struct Entry {
			detail::FileNode* node = 0;
			bool read = false;
			bool write = false;
			bool modify = false;
			bool closeOnExecute = false;
		};
		struct Node {
			std::unique_ptr<detail::FileNode> node;
			size_t user = 0;
		};

	private:
		std::vector<Entry> pFiles;
		std::vector<Node> pNodes;
		std::vector<uint8_t> pBuffer;
		std::vector<uint64_t> pCached;
		std::u8string pWDirectory;
		detail::Syscall* pSyscall = 0;
		size_t pLinkFollow = 0;

	public:
		FileIO() = default;

	private:
		int64_t fCheckRead(int64_t fd) const;
		int64_t fCheckWrite(int64_t fd) const;

	private:
		int64_t fCreateNode(std::u8string_view path, bool follow, bool append, std::function<int64_t(int64_t, detail::FileNode*, const env::FileStats&)> callback);
		int64_t fCreateFile(detail::FileNode* node, bool read, bool write, bool modify, bool closeOnExecute);
		void fDropNode(detail::FileNode* node);

	private:
		int64_t fOpenAt(int64_t dirfd, std::u8string_view path, uint64_t flags, uint64_t mode);
		int64_t fRead(detail::FileNode* node, std::function<int64_t(int64_t)> callback);
		int64_t fWrite(detail::FileNode* node) const;
		int64_t fReadLinkAt(int64_t dirfd, std::u8string_view path, env::guest_t address, uint64_t size);

	public:
		bool setup(detail::Syscall* syscall);
		detail::FileNode* link(int64_t fd);
		void drop(detail::FileNode* node);

	public:
		int64_t openat(int64_t dirfd, std::u8string_view path, uint64_t flags, uint64_t mode);
		int64_t open(std::u8string_view path, uint64_t flags, uint64_t mode);
		int64_t read(int64_t fd, env::guest_t address, uint64_t size);
		int64_t readv(int64_t fd, env::guest_t vec, uint64_t count);
		int64_t write(int64_t fd, env::guest_t address, uint64_t size);
		int64_t writev(int64_t fd, env::guest_t vec, uint64_t count);
		int64_t readlinkat(int64_t dirfd, std::u8string_view path, env::guest_t address, uint64_t size);
		int64_t readlink(std::u8string_view path, env::guest_t address, uint64_t size);
	};
}
