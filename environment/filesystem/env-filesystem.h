#pragma once

#include "../env-common.h"

namespace env {
	enum class FileType : uint8_t {
		file,
		directory,
		link,
		terminal,
		character,
		_end
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
		uint64_t id = 0;
		env::FileType type = env::FileType::_end;
		env::FileAccess access;
		bool virtualized = false;
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
	public:
		FileSystem() = default;
		FileSystem(env::FileSystem&&) = delete;
		FileSystem(const env::FileSystem&) = delete;

	private:
		bool fHandleTask(const std::u8string& task, std::function<void(json::Reader<std::u8string_view>)> callback);
		env::FileStats fParseStats(json::ObjReader<std::u8string_view> obj) const;

	public:
		/* fetch stats for path (null if does not exist) */
		void readStats(std::u8string_view path, std::function<void(const env::FileStats*)> callback);

		/* fetch stats for child of id (if name is empty, fetch own stats, returns null if does not exist) */
		void readStats(uint64_t id, std::u8string_view name, std::function<void(const env::FileStats*)> callback);

		/* fetch path for it (empty if does not exist) */
		void readPath(uint64_t id, std::function<void(std::u8string_view)> callback);

		/* fetch all children of the directory (null if does not exist) */
		void readDirectory(uint64_t id, std::function<void(const std::map<std::u8string, env::FileStats>*)> callback);

		/* mark object as read (false if does not exist) */
		void accessedObject(uint64_t id, std::function<void(bool)> callback);

		/* mark object as written (false if does not exist) */
		void changedObject(uint64_t id, std::function<void(bool)> callback);

		/* resize file to given size (false if does not exist or not file or no memory) */
		void resizeFile(uint64_t id, uint64_t size, std::function<void(bool)> callback);

		/* create file in given directory (nullopt if parent is not directory or does already exist) */
		void createFile(uint64_t id, std::u8string_view name, env::FileAccess access, std::function<void(std::optional<uint64_t>)> callback);

		/* read data from file (nullopt if not a file else number of bytes read) */
		void readFile(uint64_t id, uint64_t offset, void* data, uint64_t size, std::function<void(std::optional<uint64_t>)> callback);

		/* write data to file (nullopt if not a file else number of bytes written - will not resize file) */
		void writeFile(uint64_t id, uint64_t offset, const void* data, uint64_t size, std::function<void(std::optional<uint64_t>)> callback);
	};
}
