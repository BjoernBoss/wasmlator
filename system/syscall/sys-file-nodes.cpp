#include "../system.h"

static util::Logger logger{ u8"sys::Syscall" };

sys::detail::FileNode::FileNode(std::u8string_view path, bool directory) : pPath{ path }, pDirectory{ directory } {}
const std::u8string& sys::detail::FileNode::path() const {
	return pPath;
}
bool sys::detail::FileNode::directory() const {
	return pDirectory;
}
void sys::detail::FileNode::bind(size_t index) {
	pIndex = index;
}
size_t sys::detail::FileNode::index() const {
	return pIndex;
}

sys::detail::impl::Terminal::Terminal() : detail::FileNode{ u8"/dev/tty", false } {
	pLastRead = (pLastWrite = host::GetStampUS());
	pBlocking = true;
}
int64_t sys::detail::impl::Terminal::setup(std::function<int64_t(int64_t)> callback) {
	return callback(errCode::eSuccess);
}
int64_t sys::detail::impl::Terminal::close(std::function<int64_t(int64_t)> callback) {
	return callback(errCode::eSuccess);
}
int64_t sys::detail::impl::Terminal::read(std::vector<uint8_t>& buffer, std::function<int64_t(int64_t)> callback) {
	pLastRead = host::GetStampUS();

	/* currently not implemented */
	return callback(errCode::eIO);
}
int64_t sys::detail::impl::Terminal::write(const std::vector<uint8_t>& buffer, std::function<int64_t(int64_t)> callback) {
	pLastWrite = host::GetStampUS();
	host::GuestOut({ reinterpret_cast<const char8_t*>(buffer.data()), buffer.size() });
	return callback(buffer.size());
}
int64_t sys::detail::impl::Terminal::stats(std::function<int64_t(int64_t, const detail::NodeStats&)> callback) {
	detail::NodeStats stats;

	/* setup the stats to indicate a terminal */
	stats.isPipe = false;
	stats.isTerminal = true;
	stats.link = u8"";
	stats.name = u8"tty";
	stats.size = 0;
	stats.timeAccessedUS = pLastRead;
	stats.timeModifiedUS = pLastWrite;
	stats.type = env::FileType::none;

	return callback(errCode::eSuccess, stats);
}
void sys::detail::impl::Terminal::modify(bool blocking) {
	pBlocking = blocking;
}
