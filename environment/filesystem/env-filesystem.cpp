#include "env-filesystem.h"
#include "../process/process-access.h"

static host::Logger logger{ u8"env::filesystem" };

env::FileSystem::FileSystem() {
	/* add the blocked null-open-entry */
	pOpen.push_back(0);
}

void env::FileSystem::fCheck(uint64_t id) const {
	if (id >= pOpen.size() || pOpen[id] == 0)
		logger.fatal(u8"File id [", id, u8"] is not a valid file-system id");
}
void env::FileSystem::fHandleTask(const std::u8string& task, std::function<void(bool)> callback) {
	detail::ProcessAccess::HandleTask(task, [=](std::u8string_view response) {
		callback(!response.empty());
		});
}
void env::FileSystem::fHandleTask(const std::u8string& task, std::function<void(json::Reader<std::u8string_view>)> callback) {
	detail::ProcessAccess::HandleTask(task, [=](std::u8string_view response) {
		try {
			callback(json::Read(response));
		}
		catch (const json::Exception& e) {
			logger.fatal(u8"Malformed json-response encountered: ", e.what());
		}
		});
}
env::FileStats env::FileSystem::fReadStat(json::ObjReader<std::u8string_view> obj) const {
	env::FileStats out;
	bool tmMod = false, tmAcc = false, size = false, isDir = false;

	/* iterate over the attributes and apply them */
	for (const auto& [key, value] : obj) {
		if (key == L"mtime") {
			out.timeModified = value.unum();
			tmMod = true;
		}
		else if (key == L"atime") {
			out.timeAccessed = value.unum();
			tmAcc = true;
		}
		else if (key == L"size") {
			out.size = value.unum();
			size = true;
		}
		else if (key == L"is-dir") {
			out.isDirectory = value.boolean();
			isDir = true;
		}
	}

	/* check if all values have been received and return the parsed stats */
	if (!tmMod || !tmAcc || !size || !isDir)
		logger.fatal(u8"Received incomplete file-stats");
	return out;
}

void env::FileSystem::readStats(std::u8string path, std::function<void(bool, const env::FileStats&)> callback) {
	fHandleTask(str::u8::Build(u8"rdstats:", path), [=, this](json::Reader<std::u8string_view> resp) {
		/* check if the operation succeded */
		if (resp.isNull())
			callback(false, env::FileStats{});
		else
			callback(true, fReadStat(resp.obj()));
		});
}
void env::FileSystem::readDir(std::u8string path, std::function<void(bool, const std::vector<env::FileStats>&)> callback) {
	fHandleTask(str::u8::Build(u8"rddir:", path), [=, this](json::Reader<std::u8string_view> resp) {
		/* check if the operation succeded */
		if (resp.isNull()) {
			callback(false, {});
			return;
		}
		json::ArrReader<std::u8string_view> list = resp.arr();

		/* read all of the stats */
		std::vector<env::FileStats> stats;
		for (const auto& entry : list)
			stats.push_back(fReadStat(entry.obj()));
		callback(true, stats);
		});
}
void env::FileSystem::createDir(std::u8string path, std::function<void(bool)> callback) {
	fHandleTask(str::u8::Build(u8"mkdir:", path), callback);
}
void env::FileSystem::deleteDir(std::u8string path, std::function<void(bool)> callback) {
	fHandleTask(str::u8::Build(u8"rmdir:", path), callback);
}
void env::FileSystem::deleteFile(std::u8string path, std::function<void(bool)> callback) {
	fHandleTask(str::u8::Build(u8"rmfile:", path), callback);
}

void env::FileSystem::openFile(std::u8string path, env::FileOpen open, std::function<void(bool, uint64_t)> callback) {
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
	fHandleTask(task, [=, this](json::Reader<std::u8string_view> resp) {
		/* check if the operation failed */
		if (resp.isNull()) {
			callback(false, 0);
			return;
		}

		/* add the id-mapping */
		pOpen[slot] = resp.unum();
		callback(true, slot);
		});
}
void env::FileSystem::readFile(uint64_t id, uint64_t offset, void* data, uint64_t size, bool all, std::function<void(uint64_t)> callback) {
	fCheck(id);
	fHandleTask(str::u8::Build((all ? u8"all:" : u8"read:"), id, u8":0x", data, u8':', str::As{ U"#x", size }), [=](json::Reader<std::u8string_view> resp) {
		callback(resp.unum());
		});
}
void env::FileSystem::writeFile(uint64_t id, uint64_t offset, const void* data, uint64_t size, std::function<void(bool)> callback) {
	fCheck(id);
	fHandleTask(str::u8::Build(u8"write:", id, u8":0x", data, u8':', str::As{ U"#x", size }), callback);
}
void env::FileSystem::truncateFile(uint64_t id, uint64_t size, std::function<void(bool)> callback) {
	fCheck(id);
	fHandleTask(str::u8::Build(u8"trunc:", id, u8':', str::As{ U"#x", size }), callback);
}
void env::FileSystem::closeFile(uint64_t id, std::function<void()> callback) {
	fCheck(id);
	fHandleTask(str::u8::Build(u8"close:", id), [=, this](bool success) {
		/* cleanup the id */
		pOpen[id] = 0;
		while (pOpen.size() > 1 && pOpen.back() == 0)
			pOpen.pop_back();

		/* operation cannot fail */
		callback();
		});
}
