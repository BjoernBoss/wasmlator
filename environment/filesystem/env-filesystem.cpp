#include "env-filesystem.h"

static host::Logger logger{ u8"env::filesystem" };

env::FileSystem::FileSystem() {
	/* add the blocked null-open-entry */
	pOpen.push_back(0);
}

void env::FileSystem::fCheck() const {
	if (pActive != 0)
		logger.fatal(u8"Only one operation can be performed at a time");
}
void env::FileSystem::fCheck(uint64_t id) const {
	fCheck();
	if (id >= pOpen.size() || pOpen[id] == 0)
		logger.fatal(u8"File id [", id, u8"] is not a valid file-system id");
}
void env::FileSystem::fCompleted(uint64_t id) {
	/* check if the task was the requested task */
	if (id != pActive) {
		logger.warn(u8"Discarding unsolicited file-system task");
		return;
	}
	bool success = detail::FileSystemBridge::ReadSuccess();
	logger.debug(u8"Task [", id, u8"] resulted in: [", (success ? u8"Success" : u8"Failure"), u8']');
	pActive = 0;

	/* invoke the actual callback */
	std::function<void(bool)> callback;
	std::swap(pCallback, callback);
	callback(success);
}
void env::FileSystem::fQueueTask(std::u8string task, std::function<void(bool)> callback) {
	/* queue the task (might complete immediately, which will leave active as 0 - no chance for nested
	*	issues, as the callback will at most call fCompleted internally, no other callbacks) */
	pActive = detail::FileSystemBridge::QueueTask(task.data(), task.size());

	/* check if the task has been deferred */
	if (pActive != 0) {
		pCallback = callback;
		logger.debug(u8"Queued task [", task, u8"] as [", pActive, u8']');
		return;
	}
	logger.debug(u8"Task [", task, u8"] executed in-place");

	/* immediately invoke the callback */
	callback(detail::FileSystemBridge::ReadSuccess());
}
env::FileStats env::FileSystem::fReadStat(size_t index) const {
	detail::FileSystemBridge::SelectStat(index);
	env::FileStats out;

	/* fetch the easy attributes */
	out.timeModified = detail::FileSystemBridge::ReadStatKey(u8"mtime", 5);
	out.timeAccessed = detail::FileSystemBridge::ReadStatKey(u8"atime", 5);
	out.size = detail::FileSystemBridge::ReadStatKey(u8"size", 4);
	out.isDirectory = detail::FileSystemBridge::ReadStatKey(u8"is-dir", 6);

	/* fetch the name and free the main-allocated memory */
	char8_t* ptr = reinterpret_cast<char8_t*>(detail::FileSystemBridge::ReadStatKey(u8"name", 4));
	out.name = std::u8string(ptr);
	detail::FileSystemBridge::FreeStatKey(ptr);
	return out;
}

void env::FileSystem::readStats(std::u8string path, std::function<void(bool, const env::FileStats&)> callback) {
	fCheck();
	fQueueTask(str::u8::Build(u8"rdstats:", path), [=, this](bool success) {
		/* check if the operation succeded */
		if (!success || detail::FileSystemBridge::ReadStats() != 1)
			callback(false, env::FileStats{});
		else
			callback(true, fReadStat(0));
		});
}
void env::FileSystem::readDir(std::u8string path, std::function<void(bool, const std::vector<env::FileStats>&)> callback) {
	fCheck();
	fQueueTask(str::u8::Build(u8"rddir:", path), [=, this](bool success) {
		/* check if the operation succeded */
		if (!success) {
			callback(false, {});
			return;
		}

		/* read all of the stats */
		std::vector<env::FileStats> stats;
		for (size_t i = 0; i < detail::FileSystemBridge::ReadStats(); ++i)
			stats.push_back(fReadStat(i));
		callback(true, stats);
		});
}
void env::FileSystem::createDir(std::u8string path, std::function<void(bool)> callback) {
	fCheck();
	fQueueTask(str::u8::Build(u8"mkdir:", path), [=](bool success) {
		callback(success);
		});
}
void env::FileSystem::deleteDir(std::u8string path, std::function<void(bool)> callback) {
	fCheck();
	fQueueTask(str::u8::Build(u8"rmdir:", path), [=](bool success) {
		callback(success);
		});
}
void env::FileSystem::deleteFile(std::u8string path, std::function<void(bool)> callback) {
	fCheck();
	fQueueTask(str::u8::Build(u8"rmfile:", path), [=](bool success) {
		callback(success);
		});
}

void env::FileSystem::openFile(std::u8string path, env::FileOpen open, std::function<void(bool, uint64_t)> callback) {
	fCheck();

	/* setup the task */
	std::u8string task;
	switch (open) {
	case env::FileOpen::createAlways:
		task = u8"mkalways:";
		break;
	case env::FileOpen::openAlways:
		task = u8"opalways:";
		break;
	case env::FileOpen::createNew:
		task = u8"mknew:";
		break;
	case env::FileOpen::openExisting:
		task = u8"opexisting:";
		break;
	case env::FileOpen::truncateExisting:
		task = u8"trexisting:";
		break;
	}
	task.append(path);

	/* lookup the id-slot to be used (skip the first blocked slot) */
	uint64_t slot = 1;
	while (slot < pOpen.size() && pOpen[slot] == 0)
		++slot;
	if (slot >= pOpen.size())
		pOpen.push_back(0);

	/* queue the task to open the file */
	fQueueTask(task, [=, this](bool success) {
		/* check if the operation failed */
		if (!success) {
			callback(false, 0);
			return;
		}

		/* add the id-mapping */
		pOpen[slot] = detail::FileSystemBridge::ReadResult();
		callback(true, slot);
		});
}
void env::FileSystem::readFile(uint64_t id, uint64_t offset, void* data, uint64_t size, bool all, std::function<void(uint64_t)> callback) {
	fCheck(id);
	fQueueTask(str::u8::Build((all ? u8"all:" : u8"read:"), id, u8':', data, u8':', str::As{ U"#x", size }), [=](bool success) {
		/* operation cannot fail */
		callback(detail::FileSystemBridge::ReadResult());
		});
}
void env::FileSystem::writeFile(uint64_t id, uint64_t offset, const void* data, uint64_t size, std::function<void(bool)> callback) {
	fCheck(id);
	fQueueTask(str::u8::Build(u8"write:", id, u8':', data, u8':', str::As{ U"#x", size }), [=](bool success) {
		callback(success);
		});
}
void env::FileSystem::truncateFile(uint64_t id, uint64_t size, std::function<void(bool)> callback) {
	fCheck(id);
	fQueueTask(str::u8::Build(u8"trunc:", id, u8':', str::As{ U"#x", size }), [=](bool success) {
		callback(success);
		});
}
void env::FileSystem::closeFile(uint64_t id, std::function<void()> callback) {
	fCheck(id);
	fQueueTask(str::u8::Build(u8"close:", id), [=, this](bool success) {
		/* cleanup the id */
		pOpen[id] = 0;
		while (pOpen.size() > 1 && pOpen.back() == 0)
			pOpen.pop_back();

		/* operation cannot fail */
		callback();
		});
}
