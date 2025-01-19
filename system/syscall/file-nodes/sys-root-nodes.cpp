#include "../../system.h"

sys::detail::impl::RootFileNode::RootFileNode(detail::Syscall* syscall) : VirtualFileNode{ fs::RootOwner, fs::RootGroup, fs::RootDirPermissions }, pSyscall{ syscall } {}
int64_t sys::detail::impl::RootFileNode::virtualStats(std::function<int64_t(const env::FileStats*)> callback) const {
	env::FileStats stats;
	stats.type = env::FileType::directory;
	return callback(&stats);
}
int64_t sys::detail::impl::RootFileNode::virtualLookup(std::u8string_view name, std::function<int64_t(std::shared_ptr<detail::VirtualFileNode>)> callback) const {
	if (name == u8"dev")
		return callback(std::make_shared<impl::DevDirectory>());
	if (name == u8"proc")
		return callback(std::make_shared<impl::ProcDirectory>(pSyscall));
	return callback({});
}
