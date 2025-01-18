#include "../../system.h"

static util::Logger logger{ u8"sys::Syscall" };

sys::detail::FileNode::FileNode(std::u8string_view path) : pPath{ path } {}
const std::u8string& sys::detail::FileNode::path() const {
	return pPath;
}
std::shared_ptr<sys::detail::FileNode> sys::detail::FileNode::spawn(const std::u8string& path, std::u8string_view name) {
	return std::make_shared<impl::NullFileNode>(path);
}
int64_t sys::detail::FileNode::open(const detail::SetupConfig& config, std::function<int64_t(int64_t)> callback) {
	return callback(errCode::eIO);
}
int64_t sys::detail::FileNode::read(std::vector<uint8_t>& buffer, std::function<int64_t(int64_t)> callback) {
	return callback(errCode::eIO);
}
int64_t sys::detail::FileNode::write(const std::vector<uint8_t>& buffer, std::function<int64_t(int64_t)> callback) {
	return callback(errCode::eIO);
}
void sys::detail::FileNode::close() {}

sys::detail::VirtualFileNode::VirtualFileNode(std::u8string_view path, uint32_t owner, uint32_t group, uint16_t permissions) : FileNode{ path } {
	pLastRead = host::GetStampUS();
	pLastWrite = pLastRead;
	pOwner = owner;
	pGroup = group;
	pPermissions = permissions;
}
int64_t sys::detail::VirtualFileNode::virtualRead(std::vector<uint8_t>& buffer, std::function<int64_t(int64_t)> callback) {
	return callback(errCode::eIO);
}
int64_t sys::detail::VirtualFileNode::virtualWrite(const std::vector<uint8_t>& buffer, std::function<int64_t(int64_t)> callback) {
	return callback(errCode::eIO);
}
int64_t sys::detail::VirtualFileNode::stats(std::function<int64_t(const env::FileStats*)> callback) const {
	return virtualStats([=, this](const env::FileStats* stats) -> int64_t {
		env::FileStats out;
		out.link = stats->link;
		out.size = stats->size;
		out.type = stats->type;
		out.timeAccessedUS = pLastRead;
		out.timeModifiedUS = pLastWrite;
		out.owner = pOwner;
		out.group = pGroup;
		out.permissions.all = (fileMode::mask & pPermissions);
		return callback(&out);
		});
}
int64_t sys::detail::VirtualFileNode::read(std::vector<uint8_t>& buffer, std::function<int64_t(int64_t)> callback) {
	return virtualRead(buffer, [=, this](int64_t result) -> int64_t {
		if (result >= 0)
			pLastRead = host::GetStampUS();
		return callback(result);
		});
}
int64_t sys::detail::VirtualFileNode::write(const std::vector<uint8_t>& buffer, std::function<int64_t(int64_t)> callback) {
	return virtualWrite(buffer, [=, this](int64_t result) -> int64_t {
		if (result >= 0)
			pLastWrite = host::GetStampUS();
		return callback(result);
		});
}

sys::detail::impl::NullFileNode::NullFileNode(std::u8string_view path) : FileNode{ path } {}
int64_t sys::detail::impl::NullFileNode::stats(std::function<int64_t(const env::FileStats*)> callback) const {
	return callback(0);
}

sys::detail::impl::LinkFileNode::LinkFileNode(std::u8string_view path, std::u8string_view link, uint32_t owner, uint32_t group, uint16_t permissions) : VirtualFileNode{ path, owner, group, permissions }, pLink{ link } {}
int64_t sys::detail::impl::LinkFileNode::virtualStats(std::function<int64_t(const env::FileStats*)> callback) const {
	env::FileStats stats;

	stats.link = pLink;
	stats.size = pLink.size();
	stats.type = env::FileType::link;

	return callback(&stats);
}
