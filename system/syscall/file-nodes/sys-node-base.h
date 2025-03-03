/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025 Bjoern Boss Henrichsen */
#pragma once

#include "../../sys-common.h"

namespace sys::detail {
	class FileNode;

	struct NodeStats {
		std::u8string link;
		env::FileAccess access;
		uint64_t timeModifiedUS = 0;
		uint64_t timeAccessedUS = 0;
		uint64_t size = 0;
	};

	struct DirEntry {
		std::u8string name;
		uint64_t id = 0;
		env::FileType type = env::FileType::_end;
	};

	using SharedNode = std::shared_ptr<detail::FileNode>;

	/* Note: operations must at most throw syscall-await exception, but always call the callback */
	class FileNode : public std::enable_shared_from_this<detail::FileNode> {
	private:
		struct ExpandNamesState {
			std::function<int64_t(int64_t, const std::vector<detail::DirEntry>&)> callback;
			std::vector<std::u8string> names;
			std::vector<detail::DirEntry> out;
			size_t next = 0;
		};

	private:
		std::map<std::u8string, detail::SharedNode> pMounted;
		std::map<std::u8string, detail::SharedNode> pCache;
		detail::SharedNode pAncestor;
		std::u8string pSelf;
		uint64_t pId = 0;
		env::FileType pType = env::FileType::_end;
		bool pVirtualized = false;

	protected:
		FileNode(uint64_t id, bool virtualized, env::FileType type);

	public:
		virtual ~FileNode() = default;

	private:
		void fLinkNode(const detail::SharedNode& node, const std::u8string& name);
		int64_t fRecExpandNames(const std::shared_ptr<FileNode::ExpandNamesState>& state);

	public:
		void release();
		void mount(const std::u8string& name, const detail::SharedNode& node);
		const detail::SharedNode& ancestor() const;
		std::u8string buildPath() const;
		env::FileType type() const;
		bool virtualized() const;
		uint64_t id() const;

	protected:
		virtual int64_t makeFind(std::u8string_view name, std::function<int64_t(const detail::SharedNode&)> callback) const;
		virtual int64_t makeLookup(std::u8string_view name, std::function<int64_t(const detail::SharedNode&, const detail::NodeStats&)> callback) const;
		virtual int64_t makeCreate(std::u8string_view name, env::FileAccess access, std::function<int64_t(int64_t, const detail::SharedNode&)> callback);
		virtual int64_t makeListNames(std::function<int64_t(int64_t, const std::vector<std::u8string>&)> callback);
		virtual int64_t makeListDir(std::function<int64_t(int64_t, const std::vector<detail::DirEntry>&)> callback);

	public:
		/* generic-interactions */
		virtual int64_t stats(std::function<int64_t(const detail::NodeStats&)> callback) const = 0;
		virtual int64_t flagRead(std::function<int64_t()> callback) = 0;
		virtual int64_t flagWritten(std::function<int64_t()> callback) = 0;

	public:
		/* dir-interactions */
		int64_t lookup(std::u8string_view name, std::function<int64_t(const detail::SharedNode&, const detail::NodeStats&)> callback);
		int64_t create(std::u8string_view name, env::FileAccess access, std::function<int64_t(int64_t, const detail::SharedNode&)> callback);
		int64_t listDir(std::function<int64_t(int64_t, const std::vector<detail::DirEntry>&)> callback);

	public:
		/* file-interactions */
		virtual int64_t open(bool tryRead, bool tryWrite, bool truncate, std::function<int64_t(int64_t)> callback);
		virtual int64_t read(uint64_t offset, std::vector<uint8_t>& buffer, std::function<int64_t(int64_t)> callback);
		virtual int64_t write(uint64_t offset, const std::vector<uint8_t>& buffer, std::function<int64_t(int64_t)> callback);
		virtual int64_t close(std::function<int64_t()> callback);
	};

	/* real file-node */
	class RealFileNode : public detail::FileNode {
	protected:
		RealFileNode(uint64_t id, env::FileType type);
	};

	/* virtual file-node, which auto-applies times and access to stats */
	class VirtFileNode : public detail::FileNode {
	private:
		uint64_t pLastRead = 0;
		uint64_t pLastWrite = 0;
		env::FileAccess pAccess;

	protected:
		VirtFileNode(env::FileType type, env::FileAccess access);

	protected:
		virtual int64_t makeStats(std::function<int64_t(const std::u8string&, uint64_t)> callback) const;

	public:
		int64_t stats(std::function<int64_t(const detail::NodeStats&)> callback) const final;
		int64_t flagRead(std::function<int64_t()> callback) final;
		int64_t flagWritten(std::function<int64_t()> callback) final;
	};

	namespace impl {
		/* file-node, which provides the same link at all times */
		class LinkNode final : public detail::VirtFileNode {
		private:
			std::u8string pLink;

		public:
			LinkNode(std::u8string_view link, env::FileAccess access);

		protected:
			int64_t makeStats(std::function<int64_t(const std::u8string&, uint64_t)> callback) const final;
		};

		/* file-node, which provides an empty directory */
		class EmpyDirectory final : public detail::VirtFileNode {
		public:
			EmpyDirectory(env::FileAccess access);

		public:
			int64_t makeListNames(std::function<int64_t(int64_t, const std::vector<std::u8string>&)> callback) final;
		};
	}
}
