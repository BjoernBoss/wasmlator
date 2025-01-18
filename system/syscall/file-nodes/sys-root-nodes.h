#pragma once

#include "sys-file-nodes.h"
#include "sys-dev-nodes.h"
#include "sys-proc-nodes.h"

namespace sys::detail::impl {
	/* root file-node, which provides access to all remaining file nodes */
	class RootFileNode final : public detail::FileNode {
	private:
		detail::Syscall* pSyscall = 0;

	public:
		RootFileNode(detail::Syscall* syscall);

	public:
		int64_t stats(std::function<int64_t(const env::FileStats*)> callback) const final;
		std::shared_ptr<detail::FileNode> spawn(const std::u8string& path, std::u8string_view name) final;
	};
}
