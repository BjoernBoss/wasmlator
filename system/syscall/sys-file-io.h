#pragma once

#include "../sys-common.h"
#include "file-nodes/sys-root-node.h"

namespace sys::detail {
	static constexpr size_t MaxFollowSymLinks = 16;
	static constexpr size_t MaxFileDescriptors = 2048;

	namespace consts {
		static constexpr int fdWDirectory = -100;

		/* used by openat (opNoCTTY can be ignored) */
		static constexpr uint32_t opReadOnly = 0x000000;
		static constexpr uint32_t opWriteOnly = 0x000001;
		static constexpr uint32_t opReadWrite = 0x000002;
		static constexpr uint32_t opCreate = 0x000040;
		static constexpr uint32_t opExclusive = 0x000080;
		static constexpr uint32_t opNoCTTY = 0x000100;
		static constexpr uint32_t opTruncate = 0x000200;
		static constexpr uint32_t opDirectory = 0x010000;
		static constexpr uint32_t opNoFollow = 0x020000;
		static constexpr uint32_t opCloseOnExecute = 0x080000;
		static constexpr uint32_t opOpenOnly = 0x200000;
		static constexpr uint32_t opAppend = 0x000400;
		static constexpr uint32_t openFlagMask = consts::opReadOnly | consts::opWriteOnly |
			consts::opReadWrite | consts::opCreate | consts::opExclusive | consts::opTruncate |
			consts::opDirectory | consts::opNoFollow | consts::opCloseOnExecute |
			consts::opOpenOnly | consts::opNoCTTY | consts::opAppend;

		/* used by faccessat */
		static constexpr uint64_t accFOk = 0x00;
		static constexpr uint64_t accROk = 0x04;
		static constexpr uint64_t accWOk = 0x02;
		static constexpr uint64_t accXOk = 0x01;
		static constexpr uint64_t accEIds = 0x0200;
		static constexpr uint64_t accNoFollow = 0x0100;
		static constexpr uint64_t accEmptyPath = 0x0800;

		/* used by fcntl */
		static constexpr uint64_t fcntlDuplicateFd = 0;
		static constexpr uint64_t fcntlGetFdFlags = 1;
		static constexpr uint64_t fcntlSetFdFlags = 2;
		static constexpr uint64_t fcntlGetFileStatusFlags = 3;
		static constexpr uint64_t fcntlSetFileStatusFlags = 4;
		static constexpr uint64_t fcntlDuplicateFdCloseExec = 1030;

		/* used by fcntl */
		static constexpr uint64_t fdFlagCloseExec = 0x01;

		/* used by fcntl */
		static constexpr uint64_t fsReadOnly = 0x0000;
		static constexpr uint64_t fsWriteOnly = 0x0001;
		static constexpr uint64_t fsReadWrite = 0x0002;
		static constexpr uint64_t fsCreate = 0x0040;
		static constexpr uint64_t fsExclusive = 0x0080;
		static constexpr uint64_t fsNoCTTY = 0x0100;
		static constexpr uint64_t fsTruncate = 0x0200;
		static constexpr uint64_t fsAppend = 0x0400;
		static constexpr uint64_t fsOpenOnly = 0x200000;
		static constexpr uint64_t fsMask = consts::fsReadOnly | consts::fsWriteOnly |
			consts::fsReadWrite | consts::fsCreate | consts::fsExclusive | consts::fsNoCTTY |
			consts::fsTruncate | consts::fsAppend | consts::fsOpenOnly;
	}

	struct FdState {
		size_t instance = 0;
		bool valid = false;
		bool read = false;
		bool write = false;
		bool modify = false;
		bool append = false;
		env::FileType type = env::FileType::_end;
	};

	class FileIO {
	private:
		struct InstanceConfig {
			bool append = false;
			bool read = false;
			bool write = false;
			bool modify = false;
			struct {
				bool create = false;
				bool exclusive = false;
				bool noctty = false;
				bool truncate = false;
			} creation;
		};
		struct Instance {
			detail::SharedNode node;
			std::u8string path;
			uint64_t id = 0;
			size_t user = 0;
			env::FileType type = env::FileType::_end;
			InstanceConfig config;
		};
		struct Open {
			size_t instance = 0;
			bool used = false;
			bool closeOnExecute = false;
		};

	private:
		std::map<std::u8string, detail::SharedNode> pMounted;
		std::shared_ptr<impl::RootFileNode> pRoot;
		std::vector<Instance> pInstance;
		std::vector<Open> pOpen;
		std::vector<uint8_t> pBuffer;
		std::vector<uint64_t> pCached;
		detail::Syscall* pSyscall = 0;
		size_t pOpened = 0;
		struct {
			std::function<int64_t(int64_t, const std::u8string&, detail::SharedNode, const env::FileStats&, bool)> callback;
			size_t linkFollow = 0;
			bool follow = false;
			bool findExisting = false;
			bool effectiveIds = false;
		} pResolve;

	public:
		FileIO() = default;

	private:
		bool fCheckFd(int64_t fd) const;
		int64_t fCheckRead(int64_t fd) const;
		int64_t fCheckWrite(int64_t fd) const;
		bool fCheckAccess(const env::FileStats& stats, bool read, bool write, bool execute, bool effIds) const;
		std::pair<std::u8string, int64_t> fCheckPath(int64_t dirfd, std::u8string_view path, bool allowEmpty);

	private:
		int64_t fResolveNode(const std::u8string& path, std::function<int64_t(int64_t, const std::u8string&, detail::SharedNode, const env::FileStats&, bool)> callback);
		int64_t fResolveNext(const std::u8string& path, std::u8string_view lookup, detail::SharedNode node, const env::FileStats& stats);
		int64_t fResolveLookup(detail::SharedNode node, const std::u8string& name, const std::u8string& path, const std::u8string& remainder, const env::FileStats& stats);

	private:
		int64_t fLookupNextFd(uint64_t start, bool canFail);
		int64_t fSetupFile(detail::SharedNode node, std::u8string_view path, env::FileType type, const FileIO::InstanceConfig& config, bool closeOnExecute);
		linux::FileStats fBuildLinuxStats(const env::FileStats& stats) const;
		int64_t fRead(size_t instance, uint64_t offset, std::function<int64_t(int64_t)> callback);
		int64_t fWrite(size_t instance, uint64_t offset) const;

	private:
		int64_t fOpenAt(int64_t dirfd, std::u8string_view path, uint64_t flags, uint64_t mode);
		int64_t fReadLinkAt(int64_t dirfd, std::u8string_view path, env::guest_t address, uint64_t size);
		int64_t fAccessAt(int64_t dirfd, std::u8string_view path, uint64_t mode, uint64_t flags);

	public:
		bool setup(detail::Syscall* syscall);

	public:
		int64_t openat(int64_t dirfd, std::u8string_view path, uint64_t flags, uint64_t mode);
		int64_t open(std::u8string_view path, uint64_t flags, uint64_t mode);
		int64_t close(int64_t fd);
		int64_t read(int64_t fd, env::guest_t address, uint64_t size);
		int64_t readv(int64_t fd, env::guest_t vec, uint64_t count);
		int64_t write(int64_t fd, env::guest_t address, uint64_t size);
		int64_t writev(int64_t fd, env::guest_t vec, uint64_t count);
		int64_t readlinkat(int64_t dirfd, std::u8string_view path, env::guest_t address, uint64_t size);
		int64_t readlink(std::u8string_view path, env::guest_t address, uint64_t size);
		int64_t fstat(int64_t fd, env::guest_t address);
		int64_t fstatat(int64_t dirfd, std::u8string_view path, env::guest_t address, int64_t flags);
		int64_t access(std::u8string_view path, int64_t mode);
		int64_t faccessat(int64_t dirfd, std::u8string_view path, int64_t mode);
		int64_t faccessat2(int64_t dirfd, std::u8string_view path, int64_t mode, int64_t flags);
		int64_t ioctl(int64_t fd, uint64_t cmd, uint64_t arg);
		int64_t fcntl(int64_t fd, uint64_t cmd, uint64_t arg);

	public:
		detail::FdState fdCheck(int64_t fd) const;
		int64_t fdStats(int64_t fd, std::function<int64_t(int64_t, const env::FileStats*)> callback) const;
		int64_t fdRead(int64_t fd, uint64_t offset, uint64_t size, std::function<int64_t(const uint8_t*, uint64_t)> callback);
	};
}
