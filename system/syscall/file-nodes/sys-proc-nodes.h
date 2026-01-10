/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025-2026 Bjoern Boss Henrichsen */
#pragma once

#include "sys-node-base.h"

namespace sys::detail::impl {
	class ProcDirectory final : public detail::VirtFileNode {
	private:
		detail::Syscall* pSyscall = 0;

	public:
		ProcDirectory(detail::Syscall* syscall, env::FileAccess access);

	public:
		int64_t makeFind(std::u8string_view name, std::function<int64_t(const detail::SharedNode&)> callback) const final;
		int64_t makeListNames(std::function<int64_t(int64_t, const std::vector<std::u8string>&)> callback) final;
	};

	class ProcId final : public detail::VirtFileNode {
	private:
		detail::Syscall* pSyscall = 0;
		uint32_t pId = 0;

	public:
		ProcId(detail::Syscall* syscall, env::FileAccess access, uint32_t id);

	public:
		int64_t makeFind(std::u8string_view name, std::function<int64_t(const detail::SharedNode&)> callback) const final;
		int64_t makeListNames(std::function<int64_t(int64_t, const std::vector<std::u8string>&)> callback) final;
	};
}
