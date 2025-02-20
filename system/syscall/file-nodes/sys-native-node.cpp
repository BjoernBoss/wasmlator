#include "../../system.h"

static util::Logger logger{ u8"sys::syscall" };

sys::detail::impl::NativeFileNode::NativeFileNode(env::FileType type, detail::Syscall* syscall, uint64_t fileId) : RealFileNode{ fileId, type }, pSyscall{ syscall }, pFileId{ fileId } {}

sys::detail::NodeStats sys::detail::impl::NativeFileNode::fMakeNodeStats(const env::FileStats& stats) const {
	return detail::NodeStats{
		.link = stats.link,
		.access = stats.access,
		.timeModifiedUS = stats.timeModifiedUS,
		.timeAccessedUS = stats.timeAccessedUS,
		.size = stats.size
	};
}

int64_t sys::detail::impl::NativeFileNode::makeLookup(std::u8string_view name, std::function<int64_t(const detail::SharedNode&, const detail::NodeStats&)> callback) const {
	/* query the stats of the target name */
	env::Instance()->filesystem().readStats(pFileId, name, [this, callback](const env::FileStats* stats) {
		pSyscall->callContinue([this, callback, stats]() -> int64_t {
			/* check if the object exists */
			if (stats == 0)
				return callback({}, {});

			/* spawn the native node */
			return callback(std::make_shared<impl::NativeFileNode>(stats->type, pSyscall, stats->id), fMakeNodeStats(*stats));
			});
		});

	/* potentially defer the call */
	return pSyscall->callIncomplete();
}
int64_t sys::detail::impl::NativeFileNode::makeCreate(std::u8string_view name, env::FileAccess access, std::function<int64_t(int64_t, const detail::SharedNode&)> callback) {
	/* try to create the file-object */
	env::Instance()->filesystem().createFile(pFileId, name, access, [this, callback](std::optional<uint64_t> fileId) {
		pSyscall->callContinue([this, callback, fileId]() -> int64_t {
			/* check if the open succeeded (interrupted, as the object must have been created between lookup and create) */
			if (!fileId.has_value())
				return callback(errCode::eInterrupted, {});

			/* spawn the native node */
			return callback(errCode::eSuccess, std::make_shared<impl::NativeFileNode>(env::FileType::file, pSyscall, fileId.value()));
			});
		});

	/* potentially defer the call */
	return pSyscall->callIncomplete();
}
int64_t sys::detail::impl::NativeFileNode::makeListDir(std::function<int64_t(int64_t, const std::vector<detail::DirEntry>&)> callback) {
	/* perform the read-operation */
	env::Instance()->filesystem().readDirectory(pFileId, [this, callback](const std::map<std::u8string, env::FileStats>* map) {
		pSyscall->callContinue([callback, map]() -> int64_t {
			if (map == 0)
				return callback(errCode::eIO, {});
			std::vector<detail::DirEntry> out;
			for (const auto& [key, value] : *map)
				out.emplace_back(detail::DirEntry{ .name = key, .id = value.id, .type = value.type });
			return callback(errCode::eSuccess, out);
			});
		});

	/* potentially defer the call */
	return pSyscall->callIncomplete();
}
int64_t sys::detail::impl::NativeFileNode::stats(std::function<int64_t(const detail::NodeStats&)> callback) const {
	/* query the actual stats */
	env::Instance()->filesystem().readStats(pFileId, u8"", [this, callback](const env::FileStats* stats) {
		if (stats == 0)
			logger.fatal(u8"Failed to fetch stats of existing node [", pFileId, u8']');
		else {
			pSyscall->callContinue([this, callback, stats]() -> int64_t {
				return callback(fMakeNodeStats(*stats));
				});
		}
		});

	/* potentially defer the call */
	return pSyscall->callIncomplete();
}
int64_t sys::detail::impl::NativeFileNode::flagRead(std::function<int64_t()> callback) {
	/* perform the access-operation */
	env::Instance()->filesystem().accessedObject(pFileId, [this, callback](bool success) {
		pSyscall->callContinue([callback]() -> int64_t { return callback(); });
		});

	/* potentially defer the call */
	return pSyscall->callIncomplete();
}
int64_t sys::detail::impl::NativeFileNode::flagWritten(std::function<int64_t()> callback) {
	/* perform the access-operation */
	env::Instance()->filesystem().changedObject(pFileId, [this, callback](bool success) {
		pSyscall->callContinue([callback]() -> int64_t { return callback(); });
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
	env::Instance()->filesystem().readStats(pFileId, u8"", [this, callback](const env::FileStats* stats) {
		pSyscall->callContinue([this, callback, stats]() -> int64_t {
			return callback(stats != 0 ? errCode::eSuccess : errCode::eInterrupted);
			});
		});

	/* potentially defer the call */
	return pSyscall->callIncomplete();
}
int64_t sys::detail::impl::NativeFileNode::read(uint64_t offset, std::vector<uint8_t>& buffer, std::function<int64_t(int64_t)> callback) {
	/* perform the read-operation */
	env::Instance()->filesystem().readFile(pFileId, offset, buffer.data(), buffer.size(), [this, callback](std::optional<uint64_t> count) {
		pSyscall->callContinue([callback, count]() -> int64_t {
			if (!count.has_value())
				return callback(errCode::eIO);
			return callback(count.value());
			});
		});

	/* potentially defer the call */
	return pSyscall->callIncomplete();
}
int64_t sys::detail::impl::NativeFileNode::write(uint64_t offset, const std::vector<uint8_t>& buffer, std::function<int64_t(int64_t)> callback) {
	/* perform the write-operation */
	env::Instance()->filesystem().writeFile(pFileId, offset, buffer.data(), buffer.size(), [this, callback](std::optional<uint64_t> count) {
		pSyscall->callContinue([callback, count]() -> int64_t {
			if (!count.has_value())
				return callback(errCode::eIO);
			return callback(count.value());
			});
		});

	/* potentially defer the call */
	return pSyscall->callIncomplete();
}
