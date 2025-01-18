#pragma once

#include "sys-file-nodes.h"

namespace sys::detail::impl {
	class ProcDirectory final : public detail::VirtualFileNode {
	private:
		detail::Syscall* pSyscall = 0;

	public:
		ProcDirectory(detail::Syscall* syscall);

	public:
		std::shared_ptr<detail::FileNode> spawn(const std::u8string& path, std::u8string_view name) final;
		int64_t virtualStats(std::function<int64_t(const env::FileStats*)> callback) const final;
	};

	class ProcId final : public detail::VirtualFileNode {
	private:
		detail::Syscall* pSyscall = 0;
		uint32_t pId = 0;

	public:
		ProcId(detail::Syscall* syscall, uint32_t id);

	public:
		std::shared_ptr<detail::FileNode> spawn(const std::u8string& path, std::u8string_view name) final;
		int64_t virtualStats(std::function<int64_t(const env::FileStats*)> callback) const final;
	};
}
