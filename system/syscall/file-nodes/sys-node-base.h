#pragma once

#include "../../sys-common.h"

namespace sys::detail {
	class FileNode;

	struct DirEntry {
		std::u8string name;
		uint64_t id = 0;
		env::FileType type = env::FileType::_end;
	};
	using SharedNode = std::shared_ptr<detail::FileNode>;

	/* Note: operations must at most throw syscall-await exception, but always call the callback */
	class FileNode {
	private:
		detail::SharedNode pAncestor;

	protected:
		FileNode(const detail::SharedNode& ancestor);

	public:
		virtual ~FileNode() = default;

	public:
		const detail::SharedNode& ancestor() const;

	public:
		/* generic-interactions */
		virtual int64_t stats(std::function<int64_t(const env::FileStats*)> callback) const = 0;
		virtual int64_t linkRead(std::function<int64_t(bool)> callback);

	public:
		/* dir-interactions */
		virtual int64_t lookup(std::u8string_view name, const std::u8string& path, std::function<int64_t(std::shared_ptr<detail::FileNode>, const env::FileStats&)> callback);
		virtual int64_t create(std::u8string_view name, const std::u8string& path, env::FileAccess access, std::function<int64_t(int64_t, std::shared_ptr<detail::FileNode>)> callback);
		virtual int64_t listDir(std::function<int64_t(int64_t, const std::vector<detail::DirEntry>&)> callback);

	public:
		/* file-interactions */
		virtual int64_t open(bool truncate, std::function<int64_t(int64_t)> callback);
		virtual int64_t read(uint64_t offset, std::vector<uint8_t>& buffer, std::function<int64_t(int64_t)> callback);
		virtual int64_t write(uint64_t offset, const std::vector<uint8_t>& buffer, std::function<int64_t(int64_t)> callback);
		virtual void close();
	};

	/* virtual file-node, which auto-applies access-times to stats, and caches already created nodes */
	class VirtualFileNode : public detail::FileNode {
	private:
		std::map<std::u8string, std::shared_ptr<detail::VirtualFileNode>> pCache;
		uint64_t pLastRead = 0;
		uint64_t pLastWrite = 0;
		uint64_t pUniqueId = 0;
		env::FileAccess pAccess;

	protected:
		VirtualFileNode(const detail::SharedNode& ancestor, env::FileAccess access);

	private:
		int64_t fLookupNew(const std::u8string& name, std::function<int64_t(std::shared_ptr<detail::FileNode>, const env::FileStats&)> callback);

	public:
		virtual int64_t virtualStats(std::function<int64_t(const env::FileStats*)> callback) const = 0;
		virtual int64_t virtualLookup(std::u8string_view name, std::function<int64_t(std::shared_ptr<detail::VirtualFileNode>)> callback) const;
		virtual int64_t virtualCreate(std::u8string_view name, env::FileAccess access, std::function<int64_t(int64_t, std::shared_ptr<detail::VirtualFileNode>)> callback);
		virtual int64_t virtualListDir(std::function<int64_t(int64_t, const std::vector<detail::DirEntry>&)> callback);
		virtual int64_t virtualRead(uint64_t offset, std::vector<uint8_t>& buffer, std::function<int64_t(int64_t)> callback);
		virtual int64_t virtualWrite(uint64_t offset, const std::vector<uint8_t>& buffer, std::function<int64_t(int64_t)> callback);

	public:
		int64_t stats(std::function<int64_t(const env::FileStats*)> callback) const final;
		int64_t linkRead(std::function<int64_t(bool)> callback) final;
		int64_t lookup(std::u8string_view name, const std::u8string& path, std::function<int64_t(std::shared_ptr<detail::FileNode>, const env::FileStats&)> callback) final;
		int64_t create(std::u8string_view name, const std::u8string& path, env::FileAccess access, std::function<int64_t(int64_t, std::shared_ptr<detail::FileNode>)> callback) final;
		int64_t listDir(std::function<int64_t(int64_t, const std::vector<detail::DirEntry>&)> callback) final;
		int64_t read(uint64_t offset, std::vector<uint8_t>& buffer, std::function<int64_t(int64_t)> callback) final;
		int64_t write(uint64_t offset, const std::vector<uint8_t>& buffer, std::function<int64_t(int64_t)> callback) final;
	};

	namespace impl {
		/* file-node, which provides the same link at all times */
		class LinkNode final : public detail::VirtualFileNode {
		private:
			std::u8string pLink;

		public:
			LinkNode(const detail::SharedNode& ancestor, std::u8string_view link, env::FileAccess access);

		public:
			int64_t virtualStats(std::function<int64_t(const env::FileStats*)> callback) const final;
		};

		/* file-node, which provides an empty directory */
		class EmpyDirectory final : public detail::VirtualFileNode {
		public:
			EmpyDirectory(const detail::SharedNode& ancestor, env::FileAccess access);

		public:
			int64_t virtualStats(std::function<int64_t(const env::FileStats*)> callback) const final;
		};
	}
}
