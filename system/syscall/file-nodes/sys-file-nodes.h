#pragma once

#include "../../sys-common.h"

namespace sys::detail {
	struct SetupConfig {
		bool create = false;
		bool truncate = false;
		bool exclusive = false;
		uint32_t owner = 0;
		uint32_t group = 0;
		uint16_t permissions = 0;
	};

	namespace fs {
		static constexpr uint32_t DefaultOwner = 1001;
		static constexpr uint32_t DefaultGroup = 1001;
		static constexpr uint16_t DefaultDirPermissions = env::FileOwner(true, true, true) | env::FileGroup(true, false, true) | env::FileOther(true, false, true);
		static constexpr uint16_t DefaultFilePermissions = env::FileOwner(true, true, true) | env::FileGroup(true, false, false) | env::FileOther(false, false, false);
		static constexpr uint32_t RootOwner = 0;
		static constexpr uint32_t RootGroup = 0;
		static constexpr uint16_t RootDirPermissions = env::FileOwner(true, true, true) | env::FileGroup(true, true, true) | env::FileOther(true, false, true);
		static constexpr uint16_t RootFilePermissions = env::FileOwner(true, true, true) | env::FileGroup(true, true, true) | env::FileOther(true, false, false);
		static constexpr uint16_t ReadWrite = env::FileOwner(true, true, false) | env::FileGroup(true, true, false) | env::FileOther(true, true, false);
		static constexpr uint16_t ReadOnly = env::FileOwner(true, true, false) | env::FileGroup(true, true, false) | env::FileOther(true, true, false);
		static constexpr uint16_t ReadExecute = env::FileOwner(true, false, true) | env::FileGroup(true, false, true) | env::FileOther(true, false, true);
	}

	/* Note
	*	- operations must at most throw syscall-await exception, but always call the callback
	*	- object might be deleted within its callback
	*	- if open returns errCode::eUnknown, the entire node is discarded, and a new open-approach will be performed (to flush out old file-nodes)
	*/
	class FileNode {
	private:
		std::u8string pPath;

	protected:
		FileNode(std::u8string_view path);

	public:
		virtual ~FileNode() = default;

	public:
		const std::u8string& path() const;

	public:
		/* generic-interactions */
		virtual int64_t stats(std::function<int64_t(const env::FileStats*)> callback) const = 0;

	public:
		/* dir-interactions (spawn must never fail) */
		virtual std::shared_ptr<detail::FileNode> spawn(const std::u8string& path, std::u8string_view name);

	public:
		/* file-interactions */
		virtual int64_t open(const detail::SetupConfig& config, std::function<int64_t(int64_t)> callback);
		virtual int64_t read(std::vector<uint8_t>& buffer, std::function<int64_t(int64_t)> callback);
		virtual int64_t write(const std::vector<uint8_t>& buffer, std::function<int64_t(int64_t)> callback);
		virtual void close();
	};

	/* virtual file-node, which auto-applies access-times to stats */
	class VirtualFileNode : public detail::FileNode {
	private:
		uint64_t pLastRead = 0;
		uint64_t pLastWrite = 0;
		uint32_t pOwner = 0;
		uint32_t pGroup = 0;
		uint16_t pPermissions = 0;

	protected:
		VirtualFileNode(std::u8string_view path, uint32_t owner, uint32_t group, uint16_t permissions);

	public:
		virtual int64_t virtualStats(std::function<int64_t(const env::FileStats*)> callback) const = 0;
		virtual int64_t virtualRead(std::vector<uint8_t>& buffer, std::function<int64_t(int64_t)> callback);
		virtual int64_t virtualWrite(const std::vector<uint8_t>& buffer, std::function<int64_t(int64_t)> callback);

	public:
		int64_t stats(std::function<int64_t(const env::FileStats*)> callback) const final;
		int64_t read(std::vector<uint8_t>& buffer, std::function<int64_t(int64_t)> callback) final;
		int64_t write(const std::vector<uint8_t>& buffer, std::function<int64_t(int64_t)> callback) final;
	};

	namespace impl {
		/* null file-node, which returns null-stats to indicate non-existance */
		class NullFileNode final : public detail::FileNode {
		public:
			NullFileNode(std::u8string_view path);

		public:
			int64_t stats(std::function<int64_t(const env::FileStats*)> callback) const final;
		};

		/* file-node, which provides the same link at all times */
		class LinkFileNode final : public detail::VirtualFileNode {
		private:
			std::u8string pLink;

		public:
			LinkFileNode(std::u8string_view path, std::u8string_view link, uint32_t owner, uint32_t group, uint16_t permissions);

		public:
			int64_t virtualStats(std::function<int64_t(const env::FileStats*)> callback) const final;
		};
	}
}
