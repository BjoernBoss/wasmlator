#include "../../system.h"

static util::Logger logger{ u8"sys::syscall" };

sys::detail::impl::DevDirectory::DevDirectory() : VirtualFileNode{ u8"/dev", fs::RootOwner, fs::RootGroup, fs::RootDirPermissions } {}
std::shared_ptr<sys::detail::FileNode> sys::detail::impl::DevDirectory::spawn(const std::u8string& path, std::u8string_view name) {
	if (name == u8"tty")
		return std::make_shared<impl::DevTerminal>();
	return std::make_shared<impl::NullFileNode>(path);
}
int64_t sys::detail::impl::DevDirectory::virtualStats(std::function<int64_t(const env::FileStats*)> callback) const {
	env::FileStats stats;
	stats.type = env::FileType::directory;
	return callback(&stats);
}

sys::detail::impl::DevTerminal::DevTerminal() : VirtualFileNode{ u8"/dev/tty", fs::RootOwner, fs::RootGroup, fs::ReadWrite } {}
int64_t sys::detail::impl::DevTerminal::open(const detail::SetupConfig& config, std::function<int64_t(int64_t, uint64_t)> callback) {
	if (!config.open)
		return callback(errCode::eExists, 0);
	return callback(errCode::eSuccess, 0);
}
int64_t sys::detail::impl::DevTerminal::virtualStats(std::function<int64_t(const env::FileStats*)> callback) const {
	env::FileStats stats;
	stats.type = env::FileType::tty;
	return callback(&stats);
}
int64_t sys::detail::impl::DevTerminal::virtualRead(std::vector<uint8_t>& buffer, std::function<int64_t(int64_t)> callback) {
	logger.fatal(u8"Reading from [/dev/tty] is not yet implemented");
	return callback(errCode::eIO);
}
int64_t sys::detail::impl::DevTerminal::virtualWrite(const std::vector<uint8_t>& buffer, std::function<int64_t(int64_t)> callback) {
	host::GuestOut({ reinterpret_cast<const char8_t*>(buffer.data()), buffer.size() });
	return callback(buffer.size());
}
