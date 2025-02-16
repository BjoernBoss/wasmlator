#include "../../system.h"

static util::Logger logger{ u8"sys::syscall" };

sys::detail::impl::Terminal::Terminal(env::FileAccess access) : VirtualFileNode{ access } {}
int64_t sys::detail::impl::Terminal::open(bool truncate, std::function<int64_t(int64_t)> callback) {
	return callback(errCode::eSuccess);
}
int64_t sys::detail::impl::Terminal::virtualStats(std::function<int64_t(const env::FileStats*)> callback) const {
	env::FileStats stats;
	stats.type = env::FileType::tty;
	return callback(&stats);
}
int64_t sys::detail::impl::Terminal::virtualRead(uint64_t offset, std::vector<uint8_t>& buffer, std::function<int64_t(int64_t)> callback) {
	/* ignore any offset - as this is a character-device */
	logger.fatal(u8"Reading from [/dev/tty] is not yet implemented");
	return callback(errCode::eIO);
}
int64_t sys::detail::impl::Terminal::virtualWrite(uint64_t offset, const std::vector<uint8_t>& buffer, std::function<int64_t(int64_t)> callback) {
	/* ignore any offset - as this is a character-device */
	host::PrintOut({ reinterpret_cast<const char8_t*>(buffer.data()), buffer.size() });
	return callback(buffer.size());
}
