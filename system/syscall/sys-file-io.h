#pragma once

#include "../sys-common.h"

namespace sys::detail {
	namespace fileFlags {
		/* ignored: O_DIRECT, O_LARGEFILE, O_NOCTTY, O_SYNC, FASYNC, O_NOATIME */
		static constexpr uint32_t readOnly = 0x00000;
		static constexpr uint32_t writeOnly = 0x00001;
		static constexpr uint32_t readWrite = 0x00002;
		static constexpr uint32_t create = 0x00040;
		static constexpr uint32_t exclusive = 0x00080;
		static constexpr uint32_t truncate = 0x00200;
		static constexpr uint32_t append = 0x00400;
		static constexpr uint32_t nonBlocking = 0x00800;
		static constexpr uint32_t directory = 0x10000;
		static constexpr uint32_t noSymlinkFollow = 0x20000;
		static constexpr uint32_t closeOnExecute = 0x80000;
		static constexpr uint32_t mask = 0xb0ec3;
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
	static constexpr int workingDirectoryFD = -100;

	enum class FileState : uint8_t {
		none,
		stdIn,
		stdOut,
		errOut,
		directory
	};

	struct FileEntry {
		std::u8string path;
		detail::FileState state = detail::FileState::none;
		bool read = false;
		bool write = false;
	};

	class FileIO {
	private:
		std::vector<detail::FileEntry> pFiles;
		std::u8string pCurrent;

	public:
		FileIO() = default;

	private:
		int64_t fCheckRead(int64_t fd) const;
		int64_t fCheckWrite(int64_t fd) const;

	private:
		int64_t fOpenAt(int64_t dirfd, std::u8string_view path, uint64_t flags, uint64_t mode);
		int64_t fRead(detail::FileEntry& entry, std::vector<uint8_t>& buffer);
		int64_t fWrite(detail::FileEntry& entry, const std::vector<uint8_t>& buffer);

	public:
		bool setup(std::u8string_view currentDirectory);
		int64_t openat(int64_t dirfd, std::u8string_view path, uint64_t flags, uint64_t mode);
		int64_t open(std::u8string_view path, uint64_t flags, uint64_t mode);
		int64_t read(int64_t fd, env::guest_t address, uint64_t size);
		int64_t readv(int64_t fd, env::guest_t vec, uint64_t count);
		int64_t write(int64_t fd, env::guest_t address, uint64_t size);
		int64_t writev(int64_t fd, env::guest_t vec, uint64_t count);
	};
}
