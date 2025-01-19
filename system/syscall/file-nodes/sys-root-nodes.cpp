#include "../../system.h"

sys::detail::impl::RootFileNode::RootFileNode(detail::Syscall* syscall, env::FileAccess access) : VirtualFileNode{ access }, pSyscall{ syscall } {}
int64_t sys::detail::impl::RootFileNode::virtualStats(std::function<int64_t(const env::FileStats*)> callback) const {
	env::FileStats stats;
	stats.type = env::FileType::directory;
	return callback(&stats);
}
int64_t sys::detail::impl::RootFileNode::virtualLookup(std::u8string_view name, std::function<int64_t(std::shared_ptr<detail::VirtualFileNode>)> callback) const {
	return callback({});
}
