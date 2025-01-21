#pragma once

#include "sys-node-base.h"

namespace sys::detail::impl {
	class ProcDirectory final : public detail::VirtualFileNode {
	private:
		detail::Syscall* pSyscall = 0;

	public:
		ProcDirectory(detail::Syscall* syscall, env::FileAccess access);

	public:
		int64_t virtualStats(std::function<int64_t(const env::FileStats&)> callback) const final;
		int64_t virtualLookup(std::u8string_view name, std::function<int64_t(std::shared_ptr<detail::VirtualFileNode>)> callback) const final;
	};

	class ProcId final : public detail::VirtualFileNode {
	private:
		detail::Syscall* pSyscall = 0;
		uint32_t pId = 0;

	public:
		ProcId(detail::Syscall* syscall, env::FileAccess access, uint32_t id);

	public:
		int64_t virtualStats(std::function<int64_t(const env::FileStats&)> callback) const final;
		int64_t virtualLookup(std::u8string_view name, std::function<int64_t(std::shared_ptr<detail::VirtualFileNode>)> callback) const final;
	};
}
