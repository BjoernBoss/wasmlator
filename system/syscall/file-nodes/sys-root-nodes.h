#pragma once

#include "sys-file-nodes.h"
#include "sys-dev-nodes.h"
#include "sys-proc-nodes.h"

namespace sys::detail::impl {
	/* root file-node, which provides access to all remaining file nodes */
	class RootFileNode final : public detail::VirtualFileNode {
	private:
		detail::Syscall* pSyscall = 0;

	public:
		RootFileNode(detail::Syscall* syscall, env::FileAccess access);

	public:
		int64_t virtualStats(std::function<int64_t(const env::FileStats*)> callback) const final;
		int64_t virtualLookup(std::u8string_view name, std::function<int64_t(std::shared_ptr<detail::VirtualFileNode>)> callback) const final;
	};
}
