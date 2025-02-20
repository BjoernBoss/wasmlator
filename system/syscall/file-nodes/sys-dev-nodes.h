#pragma once

#include "sys-node-base.h"

namespace sys::detail::impl {
	class Terminal final : public detail::VirtFileNode {
	private:
		detail::Syscall* pSyscall = 0;

	public:
		Terminal(detail::Syscall* syscall, env::FileAccess access);

	public:
		int64_t open(bool truncate, std::function<int64_t(int64_t)> callback) final;
		int64_t read(uint64_t offset, std::vector<uint8_t>& buffer, std::function<int64_t(int64_t)> callback) final;
		int64_t write(uint64_t offset, const std::vector<uint8_t>& buffer, std::function<int64_t(int64_t)> callback) final;
	};
}
