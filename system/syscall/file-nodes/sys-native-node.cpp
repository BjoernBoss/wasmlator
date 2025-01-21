#include "../../system.h"

sys::detail::impl::NativeFileNode::NativeFileNode(std::u8string_view path, detail::Syscall* syscall, uint64_t openId) : pPath{ path }, pSyscall{ syscall }, pOpenId{ openId } {}

int64_t sys::detail::impl::NativeFileNode::stats(std::function<int64_t(const env::FileStats*)> callback) const {
	/* query the actual stats */
	env::Instance()->filesystem().readStats(pPath, [this, callback](const env::FileStats* stats) {
		pSyscall->callContinue([callback, stats]() -> int64_t { return callback(stats); });
		});

	/* potentially defer the call */
	return pSyscall->callIncomplete();
}
int64_t sys::detail::impl::NativeFileNode::linkRead(std::function<int64_t(bool)> callback) {
	/* perform the access-operation */
	env::Instance()->filesystem().accessedObject(pPath, [this, callback](bool success) {
		pSyscall->callContinue([callback, success]() -> int64_t { return callback(success); });
		});

	/* potentially defer the call */
	return pSyscall->callIncomplete();
}
int64_t sys::detail::impl::NativeFileNode::lookup(std::u8string_view name, const std::u8string& path, std::function<int64_t(std::shared_ptr<detail::FileNode>, const env::FileStats&)> callback) {
	/* query the stats of the target name */
	env::Instance()->filesystem().readStats(path, [this, path, callback](const env::FileStats* stats) {
		pSyscall->callContinue([this, path, callback, stats]() -> int64_t {
			/* check if the object exists */
			if (stats == 0)
				return callback({}, {});

			/* spawn the native node */
			return callback(std::make_shared<impl::NativeFileNode>(path, pSyscall, 0), *stats);
			});
		});

	/* potentially defer the call */
	return pSyscall->callIncomplete();
}
int64_t sys::detail::impl::NativeFileNode::create(std::u8string_view name, const std::u8string& path, const detail::SetupConfig& config, std::function<int64_t(int64_t, std::shared_ptr<detail::FileNode>)> callback) {
	/* configure the open-mode */
	env::FileOpen mode = env::FileOpen::openAlways;
	if (config.exclusive)
		mode = env::FileOpen::createNew;
	else if (config.truncate)
		mode = env::FileOpen::createAlways;

	/* try to create the file-object */
	env::Instance()->filesystem().openFile(path, mode, config.access, [this, path, callback](bool success, uint64_t id, const env::FileStats* stats) {
		pSyscall->callContinue([this, path, callback, success, id, stats]() -> int64_t {
			/* check if the open succeeded */
			if (!success)
				return callback((stats == 0 ? errCode::eInterrupted : errCode::eExists), {});

			/* spawn the native node */
			return callback(errCode::eSuccess, std::make_shared<impl::NativeFileNode>(path, pSyscall, id));
			});
		});

	/* potentially defer the call */
	return pSyscall->callIncomplete();
}
int64_t sys::detail::impl::NativeFileNode::open(const detail::SetupConfig& config, std::function<int64_t(int64_t)> callback) {
	/* configure the open-mode */
	env::FileOpen mode = env::FileOpen::openExisting;
	if (config.truncate)
		mode = env::FileOpen::truncateExisting;

	/* try to create the file-object */
	env::Instance()->filesystem().openFile(pPath, mode, config.access, [this, callback](bool success, uint64_t id, const env::FileStats* stats) {
		pSyscall->callContinue([this, callback, success, id, stats]() -> int64_t {
			/* check if the open succeeded */
			if (!success)
				return callback(errCode::eInterrupted);
			pOpenId = id;
			return callback(errCode::eSuccess);
			});
		});

	/* potentially defer the call */
	return pSyscall->callIncomplete();
}
int64_t sys::detail::impl::NativeFileNode::read(std::vector<uint8_t>& buffer, std::function<int64_t(int64_t)> callback) {
	/* perform the read-operation */
	env::Instance()->filesystem().readFile(pOpenId, 0, buffer.data(), buffer.size(), [this, callback](uint64_t count) {
		pSyscall->callContinue([callback, count]() -> int64_t { return callback(count); });
		});

	/* potentially defer the call */
	return pSyscall->callIncomplete();
}
int64_t sys::detail::impl::NativeFileNode::write(const std::vector<uint8_t>& buffer, std::function<int64_t(int64_t)> callback) {
	size_t size = buffer.size();

	/* perform the write-operation */
	env::Instance()->filesystem().writeFile(pOpenId, 0, buffer.data(), size, [this, callback, size](bool success) {
		pSyscall->callContinue([callback, success, size]() -> int64_t { return callback(success ? size : errCode::eIO); });
		});

	/* potentially defer the call */
	return pSyscall->callIncomplete();
}
void sys::detail::impl::NativeFileNode::close() {
	env::Instance()->filesystem().closeFile(pOpenId);
}
