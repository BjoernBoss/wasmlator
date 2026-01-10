/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025-2026 Bjoern Boss Henrichsen */
#pragma once

#include "sys-node-base.h"

namespace sys::detail::impl {
	class Terminal final : public detail::VirtFileNode {
	private:
		detail::Syscall* pSyscall = 0;

	public:
		Terminal(detail::Syscall* syscall, env::FileAccess access);

	public:
		int64_t open(bool tryRead, bool tryWrite, bool truncate, std::function<int64_t(int64_t)> callback) final;
		int64_t read(uint64_t offset, std::vector<uint8_t>& buffer, std::function<int64_t(int64_t)> callback) final;
		int64_t write(uint64_t offset, const std::vector<uint8_t>& buffer, std::function<int64_t(int64_t)> callback) final;
	};

	class URandom final : public detail::VirtFileNode {
	private:
		detail::Syscall* pSyscall = 0;

	public:
		URandom(detail::Syscall* syscall, env::FileAccess access);

	public:
		int64_t open(bool tryRead, bool tryWrite, bool truncate, std::function<int64_t(int64_t)> callback) final;
		int64_t read(uint64_t offset, std::vector<uint8_t>& buffer, std::function<int64_t(int64_t)> callback) final;
		int64_t write(uint64_t offset, const std::vector<uint8_t>& buffer, std::function<int64_t(int64_t)> callback) final;
	};

	class Null final : public detail::VirtFileNode {
	private:
		detail::Syscall* pSyscall = 0;

	public:
		Null(detail::Syscall* syscall, env::FileAccess access);

	public:
		int64_t open(bool tryRead, bool tryWrite, bool truncate, std::function<int64_t(int64_t)> callback) final;
		int64_t read(uint64_t offset, std::vector<uint8_t>& buffer, std::function<int64_t(int64_t)> callback) final;
		int64_t write(uint64_t offset, const std::vector<uint8_t>& buffer, std::function<int64_t(int64_t)> callback) final;
	};
}
