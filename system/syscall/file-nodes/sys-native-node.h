/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025 Bjoern Boss Henrichsen */
#pragma once

#include "sys-node-base.h"
#include "sys-dev-nodes.h"
#include "sys-proc-nodes.h"

namespace sys::detail::impl {
	/* file-node, which provides access to actual native file-objects */
	class NativeFileNode : public detail::RealFileNode {
	private:
		uint64_t pFileId = 0;

	protected:
		detail::Syscall* pSyscall = 0;

	public:
		NativeFileNode(env::FileType type, detail::Syscall* syscall, uint64_t fileId);

	private:
		detail::NodeStats fMakeNodeStats(const env::FileStats& stats) const;

	public:
		int64_t makeLookup(std::u8string_view name, std::function<int64_t(const detail::SharedNode&, const detail::NodeStats&)> callback) const final;
		int64_t makeCreate(std::u8string_view name, env::FileAccess access, std::function<int64_t(int64_t, const detail::SharedNode&)> callback) final;
		int64_t makeListDir(std::function<int64_t(int64_t, const std::vector<detail::DirEntry>&)> callback) final;
		int64_t stats(std::function<int64_t(const detail::NodeStats&)> callback) const final;
		int64_t flagRead(std::function<int64_t()> callback) final;
		int64_t flagWritten(std::function<int64_t()> callback) final;
		int64_t open(bool tryRead, bool tryWrite, bool truncate, std::function<int64_t(int64_t)> callback) final;
		int64_t read(uint64_t offset, std::vector<uint8_t>& buffer, std::function<int64_t(int64_t)> callback) final;
		int64_t write(uint64_t offset, const std::vector<uint8_t>& buffer, std::function<int64_t(int64_t)> callback) final;
	};
}
