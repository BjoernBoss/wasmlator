/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025-2026 Bjoern Boss Henrichsen */
#pragma once

#include "sys-node-base.h"
#include "sys-dev-nodes.h"
#include "sys-proc-nodes.h"
#include "sys-native-node.h"

namespace sys::detail::impl {
	/* root file-node, which provides access to all remaining file nodes */
	class RootFileNode final : public detail::FileNode {
	private:
		mutable std::shared_ptr<impl::NativeFileNode> pNative;
		env::FileAccess pAccess;
		detail::Syscall* pSyscall = 0;
		bool pEnabled = false;

	public:
		RootFileNode(detail::Syscall* syscall, env::FileAccess access);

	private:
		int64_t fWithNative(std::function<int64_t()> callback) const;

	public:
		void enable();

	public:
		int64_t makeLookup(std::u8string_view name, std::function<int64_t(const detail::SharedNode&, const detail::NodeStats&)> callback) const final;
		int64_t makeCreate(std::u8string_view name, env::FileAccess access, std::function<int64_t(int64_t, const detail::SharedNode&)> callback) final;
		int64_t makeListDir(std::function<int64_t(int64_t, const std::vector<detail::DirEntry>&)> callback) final;
		int64_t stats(std::function<int64_t(const detail::NodeStats&)> callback) const final;
		int64_t flagRead(std::function<int64_t()> callback) final;
		int64_t flagWritten(std::function<int64_t()> callback) final;
	};
}
