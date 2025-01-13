#pragma once

#include "../env-common.h"
#include "filesystem-access.h"

namespace env {
	static constexpr size_t MaxFollowSymLinks = 64;

	enum class FileType : uint8_t {
		none,
		file,
		directory,
		link,
		_last
	};
	struct FileStats {
		std::u8string name;
		std::u8string link;
		uint64_t timeModifiedUS = 0;
		uint64_t timeAccessedUS = 0;
		uint64_t size = 0;
		env::FileType type = env::FileType::none;
	};
	enum class FileOpen : uint8_t {
		createAlways,
		openAlways,
		createNew,
		openExisting,
		truncateExisting
	};

	class FileSystem {
		friend struct detail::FileSystemAccess;

	private:
		std::vector<std::optional<uint64_t>> pOpen;
		size_t pLinkCount = 0;

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
		void fFollowLinks(std::u8string_view path, bool first, std::function<void(std::u8string_view, const env::FileStats*)> callback);
		void fReadStats(std::u8string_view path, std::function<void(const env::FileStats*)> callback);
		void fCloseFile(uint64_t id);

	public:
		void followLinks(std::u8string_view path, std::function<void(std::u8string_view, const env::FileStats*)> callback);
		void readStats(std::u8string_view path, std::function<void(const env::FileStats*)> callback);
		void readDir(std::u8string_view path, std::function<void(bool, const std::vector<env::FileStats>&)> callback);
		void createDir(std::u8string_view path, std::function<void(bool)> callback);
		void deleteDir(std::u8string_view path, std::function<void(bool)> callback);
		void deleteFile(std::u8string_view path, std::function<void(bool)> callback);

	public:
		void openFile(std::u8string_view path, env::FileOpen open, std::function<void(bool, uint64_t, const env::FileStats*)> callback);
		void readFile(uint64_t id, uint64_t offset, void* data, uint64_t size, std::function<void(uint64_t)> callback);
		void writeFile(uint64_t id, uint64_t offset, const void* data, uint64_t size, std::function<void(bool)> callback);
		void truncateFile(uint64_t id, uint64_t size, std::function<void(bool)> callback);
		void closeFile(uint64_t id);
		bool checkFile(uint64_t id) const;
	};
}
