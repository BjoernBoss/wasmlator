#include "../../system.h"

sys::detail::impl::ProcDirectory::ProcDirectory(detail::Syscall* syscall, env::FileAccess access) : VirtualFileNode{ access }, pSyscall{ syscall } {}
int64_t sys::detail::impl::ProcDirectory::virtualStats(std::function<int64_t(const env::FileStats*)> callback) const {
	env::FileStats stats;
	stats.type = env::FileType::directory;
	return callback(&stats);
}
int64_t sys::detail::impl::ProcDirectory::virtualLookup(std::u8string_view name, std::function<int64_t(std::shared_ptr<detail::VirtualFileNode>)> callback) const {
	/* parse the id and create the id-directory */
	uint32_t id = str::ParseNumAll<uint32_t>(name, std::numeric_limits<uint32_t>::max());
	if (id != std::numeric_limits<uint32_t>::max())
		return callback(std::make_shared<impl::ProcId>(pSyscall, env::FileAccess{ fs::RootOwner, fs::RootGroup, fs::ReadExecute }, id));
	return callback({});
}

sys::detail::impl::ProcId::ProcId(detail::Syscall* syscall, env::FileAccess access, uint32_t id) : VirtualFileNode{ access }, pSyscall{ syscall }, pId{ id } {}
int64_t sys::detail::impl::ProcId::virtualStats(std::function<int64_t(const env::FileStats*)> callback) const {
	env::FileStats stats;
	stats.type = env::FileType::directory;
	return callback(&stats);
}
int64_t sys::detail::impl::ProcId::virtualLookup(std::u8string_view name, std::function<int64_t(std::shared_ptr<detail::VirtualFileNode>)> callback) const {
	if (name == u8"exe" && pId == pSyscall->process().pid)
		return callback(std::make_shared<impl::LinkNode>(pSyscall->process().path, env::FileAccess{ fs::DefOwner, fs::DefGroup, fs::All }));
	return callback({});
}
