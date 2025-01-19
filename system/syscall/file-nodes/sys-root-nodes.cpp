#include "../../system.h"

sys::detail::impl::RootFileNode::RootFileNode(detail::Syscall* syscall) : VirtualFileNode{ fs::RootOwner, fs::RootGroup, fs::RootDirPermissions }, pSyscall{ syscall } {}
int64_t sys::detail::impl::RootFileNode::virtualStats(std::function<int64_t(const env::FileStats*)> callback) const {
	env::FileStats stats;
	stats.type = env::FileType::directory;
	return callback(&stats);
}
std::shared_ptr<sys::detail::FileNode> sys::detail::impl::RootFileNode::spawn(const std::u8string& path, std::u8string_view name) {
	if (name == u8"dev")
		return std::make_shared<impl::DevDirectory>();
	if (name == u8"proc")
		return std::make_shared<impl::ProcDirectory>(pSyscall);
	return std::make_shared<impl::NullFileNode>();
}
