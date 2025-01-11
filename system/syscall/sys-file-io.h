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
	enum class PathState :uint8_t {
		invalid,
		absolute,
		relative
	};

	struct FileEntry {
		std::u8string path;
		detail::FileState state = detail::FileState::none;
	};

	class FileIO {
	private:
		std::vector<detail::FileEntry> pFiles;
		std::u8string pCurrent;

	public:
		FileIO() = default;

	private:
		detail::PathState fCheckPath(std::u8string_view path) const;
		std::u8string fMergePaths(std::u8string_view abs, std::u8string_view rel) const;

	private:
		uint64_t fOpenAt(int64_t dirfd, std::u8string path, uint64_t flags, uint64_t mode);

	public:
		bool setup(std::u8string currentDirectory);
		uint64_t openat(int64_t dirfd, std::u8string path, uint64_t flags, uint64_t mode);
		uint64_t open(std::u8string path, uint64_t flags, uint64_t mode);
	};
}
