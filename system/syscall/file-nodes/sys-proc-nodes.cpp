#include "../../system.h"

sys::detail::impl::ProcDirectory::ProcDirectory(detail::Syscall* syscall) : VirtualFileNode{ fs::RootOwner, fs::RootGroup, fs::ReadExecute }, pSyscall{ syscall } {}
std::shared_ptr<sys::detail::FileNode> sys::detail::impl::ProcDirectory::spawn(const std::u8string& path, std::u8string_view name) {
	if (name == u8"self")
		return std::make_shared<impl::LinkFileNode>(str::u8::Int(pSyscall->config().pid), fs::RootOwner, fs::RootGroup, fs::ReadOnly);
	uint32_t id = str::ParseNumAll<uint32_t>(name, std::numeric_limits<uint32_t>::max());
	if (id == std::numeric_limits<uint32_t>::max())
		return std::make_shared<impl::NullFileNode>();
	return std::make_shared<impl::ProcId>(pSyscall, id);
}
int64_t sys::detail::impl::ProcDirectory::virtualStats(std::function<int64_t(const env::FileStats*)> callback) const {
	env::FileStats stats;
	stats.type = env::FileType::directory;
	return callback(&stats);
}

sys::detail::impl::ProcId::ProcId(detail::Syscall* syscall, uint32_t id) : VirtualFileNode{ fs::RootOwner, fs::RootGroup, fs::ReadExecute }, pSyscall{ syscall } {}
std::shared_ptr<sys::detail::FileNode> sys::detail::impl::ProcId::spawn(const std::u8string& path, std::u8string_view name) {
	if (name == u8"exe" && pId == pSyscall->config().pid)
		return std::make_shared<impl::LinkFileNode>(pSyscall->config().path, fs::RootOwner, fs::RootGroup, fs::ReadOnly);
	return std::make_shared<impl::NullFileNode>();
}
int64_t sys::detail::impl::ProcId::virtualStats(std::function<int64_t(const env::FileStats*)> callback) const {
	env::FileStats stats;
	stats.type = env::FileType::directory;
	return callback(&stats);
}
