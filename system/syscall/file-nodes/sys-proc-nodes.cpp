#include "../../system.h"

sys::detail::impl::ProcDirectory::ProcDirectory(detail::Syscall* syscall) : VirtualFileNode{ fs::RootOwner, fs::RootGroup, fs::ReadExecute }, pSyscall{ syscall } {}
int64_t sys::detail::impl::ProcDirectory::virtualStats(std::function<int64_t(const env::FileStats*)> callback) const {
	env::FileStats stats;
	stats.type = env::FileType::directory;
	return callback(&stats);
}
int64_t sys::detail::impl::ProcDirectory::virtualLookup(std::u8string_view name, std::function<int64_t(std::shared_ptr<detail::VirtualFileNode>)> callback) const {
	if (name == u8"self")
		return callback(std::make_shared<impl::LinkFileNode>(str::u8::Int(pSyscall->config().pid), fs::RootOwner, fs::RootGroup, fs::ReadOnly));

	/* parse the id and create the id-directory */
	uint32_t id = str::ParseNumAll<uint32_t>(name, std::numeric_limits<uint32_t>::max());
	if (id != std::numeric_limits<uint32_t>::max())
		return callback(std::make_shared<impl::ProcId>(pSyscall, id));
	return callback({});
}

sys::detail::impl::ProcId::ProcId(detail::Syscall* syscall, uint32_t id) : VirtualFileNode{ fs::RootOwner, fs::RootGroup, fs::ReadExecute }, pSyscall{ syscall }, pId{ id } {}
int64_t sys::detail::impl::ProcId::virtualStats(std::function<int64_t(const env::FileStats*)> callback) const {
	env::FileStats stats;
	stats.type = env::FileType::directory;
	return callback(&stats);
}
int64_t sys::detail::impl::ProcId::virtualLookup(std::u8string_view name, std::function<int64_t(std::shared_ptr<detail::VirtualFileNode>)> callback) const {
	if (name == u8"exe" && pId == pSyscall->config().pid)
		return callback(std::make_shared<impl::LinkFileNode>(pSyscall->config().path, fs::RootOwner, fs::RootGroup, fs::ReadOnly));
	return callback({});
}
