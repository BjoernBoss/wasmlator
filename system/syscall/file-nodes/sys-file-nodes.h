#pragma once

#include "../../sys-common.h"

namespace sys::detail {
	struct SetupConfig {
		env::FileAccess access;
		bool truncate = false;
		bool exclusive = false;
	};

	namespace fs {
		static constexpr uint32_t DefOwner = 1001;
		static constexpr uint32_t DefGroup = 1001;
		static constexpr uint32_t RootOwner = 0;
		static constexpr uint32_t RootGroup = 0;

		static constexpr uint16_t OwnerAllElseRW = env::FileOwner(true, true, true) | env::FileGroup(true, false, true) | env::FileOther(true, false, true);
		static constexpr uint16_t All = env::FileOwner(true, true, true) | env::FileGroup(true, true, true) | env::FileOther(true, true, true);
		static constexpr uint16_t ReadWrite = env::FileOwner(true, true, false) | env::FileGroup(true, true, false) | env::FileOther(true, true, false);
		static constexpr uint16_t ReadOnly = env::FileOwner(true, true, false) | env::FileGroup(true, true, false) | env::FileOther(true, true, false);
		static constexpr uint16_t ReadExecute = env::FileOwner(true, false, true) | env::FileGroup(true, false, true) | env::FileOther(true, false, true);
	}

	/* Note
	*	- operations must at most throw syscall-await exception, but always call the callback
	*	- object might be deleted within its callback
	*/
	class FileNode {
	protected:
		FileNode() = default;

	public:
		virtual ~FileNode() = default;

	public:
		/* generic-interactions */
		virtual int64_t stats(std::function<int64_t(const env::FileStats*)> callback) const = 0;
		virtual void linkRead();

	public:
		/* dir-interactions */
		virtual int64_t lookup(std::u8string_view name, std::function<int64_t(std::shared_ptr<detail::FileNode>, const env::FileStats*)> callback);
		virtual int64_t create(std::u8string_view name, const detail::SetupConfig& config, std::function<int64_t(int64_t, std::shared_ptr<detail::FileNode>)> callback);

	public:
		/* file-interactions */
		virtual int64_t open(const detail::SetupConfig& config, std::function<int64_t(int64_t)> callback);
		virtual int64_t read(std::vector<uint8_t>& buffer, std::function<int64_t(int64_t)> callback);
		virtual int64_t write(const std::vector<uint8_t>& buffer, std::function<int64_t(int64_t)> callback);
		virtual void close();
	};
	using SharedNode = std::shared_ptr<detail::FileNode>;

	/* virtual file-node, which auto-applies access-times to stats, and caches already created nodes */
	class VirtualFileNode : public detail::FileNode {
	private:
		std::map<std::u8string, std::shared_ptr<detail::VirtualFileNode>> pCache;
		uint64_t pLastRead = 0;
		uint64_t pLastWrite = 0;
		env::FileAccess pAccess;

	protected:
		VirtualFileNode(env::FileAccess access);

	private:
		int64_t fLookupNew(const std::u8string& name, std::function<int64_t(std::shared_ptr<detail::FileNode>, const env::FileStats*)> callback);
		int64_t fCreateNew(const std::u8string& name, const detail::SetupConfig& config, std::function<int64_t(int64_t, std::shared_ptr<detail::FileNode>)> callback);

	public:
		virtual int64_t virtualStats(std::function<int64_t(const env::FileStats*)> callback) const = 0;
		virtual int64_t virtualLookup(std::u8string_view name, std::function<int64_t(std::shared_ptr<detail::VirtualFileNode>)> callback) const;
		virtual int64_t virtualCreate(std::u8string_view name, const detail::SetupConfig& config, std::function<int64_t(int64_t, std::shared_ptr<detail::VirtualFileNode>)> callback);
		virtual int64_t virtualRead(std::vector<uint8_t>& buffer, std::function<int64_t(int64_t)> callback);
		virtual int64_t virtualWrite(const std::vector<uint8_t>& buffer, std::function<int64_t(int64_t)> callback);

	public:
		int64_t stats(std::function<int64_t(const env::FileStats*)> callback) const final;
		void linkRead() final;
		int64_t lookup(std::u8string_view name, std::function<int64_t(std::shared_ptr<detail::FileNode>, const env::FileStats*)> callback) final;
		int64_t create(std::u8string_view name, const detail::SetupConfig& config, std::function<int64_t(int64_t, std::shared_ptr<detail::FileNode>)> callback) final;
		int64_t read(std::vector<uint8_t>& buffer, std::function<int64_t(int64_t)> callback) final;
		int64_t write(const std::vector<uint8_t>& buffer, std::function<int64_t(int64_t)> callback) final;
	};

	namespace impl {
		/* file-node, which provides the same link at all times */
		class LinkNode final : public detail::VirtualFileNode {
		private:
			std::u8string pLink;

		public:
			LinkNode(std::u8string_view link, env::FileAccess access);

		public:
			int64_t virtualStats(std::function<int64_t(const env::FileStats*)> callback) const final;
		};

		/* file-node, which provides an empty directory */
		class EmpyDirectory final : public detail::VirtualFileNode {
		public:
			EmpyDirectory(env::FileAccess access);

		public:
			int64_t virtualStats(std::function<int64_t(const env::FileStats*)> callback) const final;
		};
	}
}
