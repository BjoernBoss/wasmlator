#pragma once

#include "../env-common.h"
#include "filesystem-access.h"

namespace env {
	enum class FileType : uint8_t {
		none,
		file,
		directory,
		link,
		pipe,
		tty,
		_last
	};
	enum class FileOpen : uint8_t {
		createAlways,
		openAlways,
		createNew,
		openExisting,
		truncateExisting
	};

	static constexpr uint16_t fileModeMask = 0x01ff;

	struct FileAccess {
	public:
		uint32_t owner = 0;
		uint32_t group = 0;
		union {
			uint16_t all = 0;
			struct {
				uint16_t xOther : 1;
				uint16_t wOther : 1;
				uint16_t rOther : 1;
				uint16_t xGroup : 1;
				uint16_t wGroup : 1;
				uint16_t rGroup : 1;
				uint16_t xOwner : 1;
				uint16_t wOwner : 1;
				uint16_t rOwner : 1;
			};
		} permissions;

	public:
		FileAccess() = default;
		FileAccess(uint32_t o, uint32_t g, uint16_t p) : owner{ o }, group{ g }, permissions{ uint16_t(p & env::fileModeMask) } {}
	};
	struct FileStats {
		std::u8string link;
		uint64_t timeModifiedUS = 0;
		uint64_t timeAccessedUS = 0;
		uint64_t size = 0;
		env::FileType type = env::FileType::none;
		env::FileAccess access;
	};

	/* construct the permissions for owner */
	constexpr uint16_t FileOwner(bool r, bool w, bool x) {
		return (r ? 0x100 : 0x000) | (w ? 0x080 : 0x000) | (x ? 0x040 : 0x000);
	}

	/* construct the permissions for group */
	constexpr uint16_t FileGroup(bool r, bool w, bool x) {
		return (r ? 0x020 : 0x000) | (w ? 0x010 : 0x000) | (x ? 0x008 : 0x000);
	}

	/* construct the permissions for others */
	constexpr uint16_t FileOther(bool r, bool w, bool x) {
		return (r ? 0x004 : 0x000) | (w ? 0x002 : 0x000) | (x ? 0x001 : 0x000);
	}

	/* Note: all paths are expected to be fully qualified real absolute paths (symlinks will not be followed along the path)
	*	Note: env::FileSystem will only ever produce file/directory/link */
	class FileSystem {
		friend struct detail::FileSystemAccess;

	private:
		std::vector<std::optional<uint64_t>> pOpen;

	public:
		FileSystem() = default;
		FileSystem(env::FileSystem&&) = delete;
		FileSystem(const env::FileSystem&) = delete;

	private:
		bool fCheck(uint64_t id) const;
		std::u8string fPrepare(std::u8string_view path) const;
		bool fHandleTask(const std::u8string& task, std::function<void(bool)> callback);
		bool fHandleTask(const std::u8string& task, std::function<void(json::Reader<std::u8string_view>)> callback);
		env::FileStats fParseStats(json::ObjReader<std::u8string_view> obj) const;
		void fCloseAll();

	private:
		void fReadStats(std::u8string_view path, std::function<void(const env::FileStats*)> callback);
		void fCloseFile(uint64_t id);

	public:
		void readStats(std::u8string_view path, std::function<void(const env::FileStats*)> callback);
		void readDir(std::u8string_view path, std::function<void(bool, const std::vector<std::u8string>&)> callback);
		void createDir(std::u8string_view path, std::function<void(bool)> callback);
		void deleteDir(std::u8string_view path, std::function<void(bool)> callback);
		void deleteFile(std::u8string_view path, std::function<void(bool)> callback);

	public:
		void openFile(std::u8string_view path, env::FileOpen open, env::FileAccess access, std::function<void(bool, uint64_t, const env::FileStats*)> callback);
		void readFile(uint64_t id, uint64_t offset, void* data, uint64_t size, std::function<void(uint64_t)> callback);
		void writeFile(uint64_t id, uint64_t offset, const void* data, uint64_t size, std::function<void(bool)> callback);
		void truncateFile(uint64_t id, uint64_t size, std::function<void(bool)> callback);
		void closeFile(uint64_t id);
		bool checkFile(uint64_t id) const;
	};
}
