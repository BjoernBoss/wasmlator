#include "../../system.h"

sys::detail::impl::NativeFileNode::NativeFileNode(detail::Syscall* syscall, uint64_t fileId) : pSyscall{ syscall }, pFileId{ fileId } {}

int64_t sys::detail::impl::NativeFileNode::stats(std::function<int64_t(const env::FileStats*)> callback) const {
	/* query the actual stats */
	env::Instance()->filesystem().readStats(pFileId, [this, callback](const env::FileStats* stats) {
		pSyscall->callContinue([callback, stats]() -> int64_t { return callback(stats); });
		});

	/* potentially defer the call */
	return pSyscall->callIncomplete();
}
int64_t sys::detail::impl::NativeFileNode::getPath(std::function<int64_t(std::u8string_view)> callback) {
	/* perform the query-operation */
	env::Instance()->filesystem().readPath(pFileId, [this, callback](std::u8string_view path) {
		pSyscall->callContinue([callback, path]() -> int64_t { return callback(path); });
		});

	/* potentially defer the call */
	return pSyscall->callIncomplete();
}
int64_t sys::detail::impl::NativeFileNode::linkRead(std::function<int64_t(bool)> callback) {
	/* perform the access-operation */
	env::Instance()->filesystem().accessedObject(pFileId, [this, callback](bool success) {
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
			return callback(std::make_shared<impl::NativeFileNode>(pSyscall, stats->id), *stats);
			});
		});

	/* potentially defer the call */
	return pSyscall->callIncomplete();
}
int64_t sys::detail::impl::NativeFileNode::create(std::u8string_view name, const std::u8string& path, env::FileAccess access, std::function<int64_t(int64_t, std::shared_ptr<detail::FileNode>)> callback) {
	/* try to create the file-object */
	env::Instance()->filesystem().createFile(pFileId, name, access, [this, callback](std::optional<uint64_t> fileId) {
		pSyscall->callContinue([this, callback, fileId]() -> int64_t {
			/* check if the open succeeded */
			if (!fileId.has_value())
				return callback(errCode::eInterrupted, {});

			/* spawn the native node */
			return callback(errCode::eSuccess, std::make_shared<impl::NativeFileNode>(pSyscall, fileId.value()));
			});
		});

	/* potentially defer the call */
	return pSyscall->callIncomplete();
}
int64_t sys::detail::impl::NativeFileNode::open(bool truncate, std::function<int64_t(int64_t)> callback) {
	/* check if the file should be truncated */
	if (truncate) {
		env::Instance()->filesystem().resizeFile(pFileId, 0, [this, callback](bool success) {
			pSyscall->callContinue([this, callback, success]() -> int64_t {
				return callback(success ? errCode::eSuccess : errCode::eInterrupted);
				});
			});
	}

	/* simply validate the existance of the file */
	env::Instance()->filesystem().readStats(pFileId, [this, callback](const env::FileStats* stats) {
		pSyscall->callContinue([this, callback, stats]() -> int64_t {
			return callback(stats != 0 ? errCode::eSuccess : errCode::eInterrupted);
			});
		});

	/* potentially defer the call */
	return pSyscall->callIncomplete();
}
int64_t sys::detail::impl::NativeFileNode::read(std::vector<uint8_t>& buffer, std::function<int64_t(int64_t)> callback) {
	/* perform the read-operation */
	env::Instance()->filesystem().readFile(pFileId, 0, buffer.data(), buffer.size(), [this, callback](std::optional<uint64_t> count) {
		pSyscall->callContinue([callback, count]() -> int64_t {
			if (!count.has_value())
				return callback(errCode::eIO);
			return callback(count.value());
			});
		});

	/* potentially defer the call */
	return pSyscall->callIncomplete();
}
int64_t sys::detail::impl::NativeFileNode::write(const std::vector<uint8_t>& buffer, std::function<int64_t(int64_t)> callback) {
	/* perform the write-operation */
	env::Instance()->filesystem().writeFile(pFileId, 0, buffer.data(), buffer.size(), [this, callback](std::optional<uint64_t> count) {
		pSyscall->callContinue([callback, count]() -> int64_t {
			if (!count.has_value())
				return callback(errCode::eIO);
			return callback(count.value());
			});
		});

	/* potentially defer the call */
	return pSyscall->callIncomplete();
}
