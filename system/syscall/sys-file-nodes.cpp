#include "../system.h"

static util::Logger logger{ u8"sys::Syscall" };

sys::detail::FileNode::FileNode(bool writable) : pWritable{ writable } {}
void sys::detail::FileNode::bindConfigure(size_t index, std::u8string_view directory) {
	pDirectory = std::u8string{ directory };
	pIndex = index;
}
const std::u8string& sys::detail::FileNode::directory() const {
	return pDirectory;
}
bool sys::detail::FileNode::writable() const {
	return pWritable;
}
size_t sys::detail::FileNode::index() const {
	return pIndex;
}


sys::detail::impl::Terminal::Terminal() : detail::FileNode{ true }, pBlocking{ true }, pLastRead{ host::GetStampUS() }, pLastWrite{ host::GetStampUS() } {}
env::FileStats sys::detail::impl::Terminal::fStats() const {
	env::FileStats stats;

	stats.link = u8"";
	stats.size = 0;
	stats.timeAccessedUS = pLastRead;
	stats.timeModifiedUS = pLastWrite;
	stats.type = env::FileType::tty;

	return stats;
}
int64_t sys::detail::impl::Terminal::setup(std::function<int64_t(int64_t, const env::FileStats&)> callback) {
	return callback(errCode::eSuccess, fStats());
}
int64_t sys::detail::impl::Terminal::close(std::function<int64_t()> callback) {
	return callback();
}
int64_t sys::detail::impl::Terminal::read(std::vector<uint8_t>& buffer, std::function<int64_t(int64_t)> callback) {
	pLastRead = host::GetStampUS();
	logger.fatal(u8"Reading from [/dev/tty] is not yet implemented");
	return callback(errCode::eIO);
}
int64_t sys::detail::impl::Terminal::write(const std::vector<uint8_t>& buffer, std::function<int64_t(int64_t)> callback) {
	pLastWrite = host::GetStampUS();
	host::GuestOut({ reinterpret_cast<const char8_t*>(buffer.data()), buffer.size() });
	return callback(buffer.size());
}
int64_t sys::detail::impl::Terminal::stats(std::function<int64_t(const env::FileStats&)> callback) {
	return callback(fStats());
}
void sys::detail::impl::Terminal::modify(bool blocking) {
	pBlocking = blocking;
}


sys::detail::impl::ProcDirectory::ProcDirectory(detail::Syscall* syscall, std::u8string_view path) : detail::FileNode{ false }, pSyscall{ syscall } { fMatchType(path); }
void sys::detail::impl::ProcDirectory::fMatchType(std::u8string_view path) {
	if (path == u8"/proc")
		pValue = Value::root;
	else if (path == u8"/proc/self")
		pValue = Value::self;
	else if (path == u8"/proc/self/exe")
		pValue = Value::selfExe;

	/* configure the error properly */
	else {
		path = util::SplitPath(path).first;
		if (path == u8"/proc" || path == u8"/proc/self")
			pValue = Value::noEntry;
		else
			pValue = Value::notDirectory;
	}
}
env::FileStats sys::detail::impl::ProcDirectory::fStats() const {
	env::FileStats stats;

	stats.link = u8"";
	stats.timeAccessedUS = pLastRead;
	stats.timeModifiedUS = 0;
	stats.size = 0;

	switch (pValue) {
	case Value::root:
		stats.type = env::FileType::directory;
		break;
	case Value::self:
		stats.type = env::FileType::directory;
		break;
	case Value::selfExe:
		stats.type = env::FileType::link;
		stats.link = pSyscall->config().path;
		stats.size = stats.link.size();
		break;
	default:
		logger.fatal(u8"Unknown value type for [/proc] encountered");
		break;
	}

	return stats;
}
int64_t sys::detail::impl::ProcDirectory::setup(std::function<int64_t(int64_t, const env::FileStats&)> callback) {
	if (pValue == Value::noEntry)
		return callback(errCode::eNoEntry, {});
	else if (pValue == Value::notDirectory)
		return callback(errCode::eNotDirectory, {});
	return callback(errCode::eSuccess, fStats());
}
int64_t sys::detail::impl::ProcDirectory::close(std::function<int64_t()> callback) {
	return callback();
}
int64_t sys::detail::impl::ProcDirectory::read(std::vector<uint8_t>& buffer, std::function<int64_t(int64_t)> callback) {
	pLastRead = host::GetStampUS();
	logger.fatal(u8"Reading from [/proc] is not yet implemented");
	return callback(errCode::eIO);
}
int64_t sys::detail::impl::ProcDirectory::write(const std::vector<uint8_t>& buffer, std::function<int64_t(int64_t)> callback) {
	return errCode::eIO;
}
int64_t sys::detail::impl::ProcDirectory::stats(std::function<int64_t(const env::FileStats&)> callback) {
	return callback(fStats());
}
void sys::detail::impl::ProcDirectory::modify(bool blocking) {}
