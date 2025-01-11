#pragma once

#include "../env-common.h"
#include "filesystem-bridge.h"

namespace env {
	struct FileStats {
		std::u8string name;
		uint64_t timeModified = 0;
		uint64_t timeAccessed = 0;
		uint64_t size = 0;
		bool isDirectory = false;
	};
	enum class FileOpen : uint8_t {
		createAlways,
		openAlways,
		createNew,
		openExisting,
		truncateExisting
	};

	class FileSystem {
		friend detail::FileSystemBridge;
	private:
		std::function<void(bool)> pCallback;
		std::vector<uint64_t> pOpen;
		uint64_t pActive = 0;

	public:
		FileSystem();
		FileSystem(env::FileSystem&&) = delete;
		FileSystem(const env::FileSystem&) = delete;

	private:
		void fCheck() const;
		void fCheck(uint64_t id) const;
		void fCompleted(uint64_t id);
		void fQueueTask(std::u8string task, std::function<void(bool)> callback);
		env::FileStats fReadStat(size_t index) const;

	public:
		void readStats(std::u8string path, std::function<void(bool, const env::FileStats&)> callback);
		void readDir(std::u8string path, std::function<void(bool, const std::vector<env::FileStats>&)> callback);
		void createDir(std::u8string path, std::function<void(bool)> callback);
		void deleteDir(std::u8string path, std::function<void(bool)> callback);
		void deleteFile(std::u8string path, std::function<void(bool)> callback);

	public:
		void openFile(std::u8string path, env::FileOpen open, std::function<void(bool, uint64_t)> callback);
		void readFile(uint64_t id, uint64_t offset, void* data, uint64_t size, bool all, std::function<void(uint64_t)> callback);
		void writeFile(uint64_t id, uint64_t offset, const void* data, uint64_t size, std::function<void(bool)> callback);
		void truncateFile(uint64_t id, uint64_t size, std::function<void(bool)> callback);
		void closeFile(uint64_t id, std::function<void()> callback);
	};
}
