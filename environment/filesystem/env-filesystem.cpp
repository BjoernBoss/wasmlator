#include "env-filesystem.h"
#include "../process/process-access.h"

static host::Logger logger{ u8"env::filesystem" };

bool env::FileSystem::fCheck(uint64_t id) const {
	return (id < pOpen.size() && pOpen[id].has_value());
}
std::u8string env::FileSystem::fPrepare(std::u8string_view path) const {
	/* ensure that the path is absolute */
	if (util::TestPath(path) != util::PathState::absolute)
		logger.fatal(u8"Path [", path, u8"] is not a valid absolute path");

	/* return the canonicalized path */
	return util::CanonicalPath(path);
}
bool env::FileSystem::fHandleTask(const std::u8string& task, std::function<void(bool)> callback) {
	return detail::ProcessAccess::HandleTask(task, [=](std::u8string_view response, bool) {
		callback(!response.empty());
		});
}
bool env::FileSystem::fHandleTask(const std::u8string& task, std::function<void(json::Reader<std::u8string_view>)> callback) {
	return detail::ProcessAccess::HandleTask(task, [=](std::u8string_view response, bool) {
		if (response.empty())
			response = u8"null";
		try {
			callback(json::Read(response));
		}
		catch (const json::Exception& e) {
			logger.fatal(u8"Malformed json-response encountered: ", e.what());
		}
		});
}
env::FileStats env::FileSystem::fParseStats(json::ObjReader<std::u8string_view> obj) const {
	env::FileStats out;
	bool tmMod = false, tmAcc = false, size = false, type = false, name = false, link = false;

	/* iterate over the attributes and apply them */
	for (const auto& [key, value] : obj) {
		if (key == L"mtime_us") {
			out.timeModifiedUS = value.unum();
			tmMod = true;
		}
		else if (key == L"atime_us") {
			out.timeAccessedUS = value.unum();
			tmAcc = true;
		}
		else if (key == L"size") {
			out.size = value.unum();
			size = true;
		}
		else if (key == L"type") {
			const std::wstring& _type = value.str();
			if (_type == L"file")
				out.type = env::FileType::file;
			else if (_type == L"dir")
				out.type = env::FileType::directory;
			else if (_type == L"link")
				out.type = env::FileType::link;
			else
				logger.fatal(u8"Received invalid file-type [", _type, u8']');
			type = true;
		}
		else if (key == L"name") {
			out.name = str::u8::To(value.str());
			name = true;
		}
		else if (key == L"link") {
			out.link = str::u8::To(value.str());
			link = true;
		}
	}

	/* check if all values have been received and return the parsed stats */
	if (!tmMod || !tmAcc || !size || !type || !name || !link)
		logger.fatal(u8"Received incomplete file-stats");
	return out;
}
void env::FileSystem::fCloseAll() {
	/* the value must exist, as close would otherwise have released it */
	while (!pOpen.empty())
		fCloseFile(pOpen.back().value());
}

void env::FileSystem::fFollowLinks(std::u8string_view path, bool first, std::function<void(std::u8string_view, const env::FileStats*)> callback) {
	/* check if this is the first following and reset the counter */
	if (first)
		pLinkCount = 0;

	/* check if the follow limit count has been reached */
	else if (++pLinkCount >= env::MaxFollowSymLinks) {
		logger.error(u8"Link following limit has been reached");
		callback(u8"", 0);
		return;
	}

	/* read the stats and check if they need to be expanded */
	fReadStats(path, [path = std::u8string(path), callback, this](const env::FileStats* stats) {
		/* check if the reading failed */
		if (stats == 0) {
			callback(u8"", 0);
			return;
		}

		/* check if the end of the link-chain has been reached */
		if (stats->type != env::FileType::link) {
			callback(path, stats);
			return;
		}

		/* start following the link again */
		fFollowLinks(util::MergePaths(util::SplitPath(path).first, stats->link), false, callback);
		});
}
void env::FileSystem::fReadStats(std::u8string_view path, std::function<void(const env::FileStats*)> callback) {
	fHandleTask(str::u8::Build(u8"stats:", path), [=, this](json::Reader<std::u8string_view> resp) {
		/* check if the operation succeded */
		if (resp.isNull())
			callback(0);

		/* pass the stats to the callback */
		else {
			env::FileStats stats = fParseStats(resp.obj());
			callback(&stats);
		}
		});
}
void env::FileSystem::fCloseFile(uint64_t id) {
	/* queue the task to close the file and cleanup the id (operation cannot fail) */
	bool inplace = fHandleTask(str::u8::Build(u8"close:", pOpen[id].value()), [=, this](bool success) {
		/* cleanup the id */
		pOpen[id] = std::nullopt;
		while (!pOpen.empty() && !pOpen.back().has_value())
			pOpen.pop_back();
		});

	/* check if the closing was peformed in-place (necessary) */
	if (!inplace)
		logger.fatal(u8"Closing of file [", id, u8"] was not performed inplace");
}

void env::FileSystem::followLinks(std::u8string_view path, std::function<void(std::u8string_view, const env::FileStats*)> callback) {
	logger.debug(u8"Resolving symlink [", path, u8']');
	std::u8string actual = fPrepare(path);
	fFollowLinks(actual, true, callback);
}
void env::FileSystem::readStats(std::u8string_view path, std::function<void(const env::FileStats*)> callback) {
	logger.debug(u8"Reading stats of [", path, u8']');
	std::u8string actual = fPrepare(path);
	fReadStats(actual, callback);
}
void env::FileSystem::readDir(std::u8string_view path, std::function<void(bool, const std::vector<env::FileStats>&)> callback) {
	logger.debug(u8"Reading directory [", path, u8']');
	std::u8string actual = fPrepare(path);
	fHandleTask(str::u8::Build(u8"list:", actual), [=, this](json::Reader<std::u8string_view> resp) {
		/* check if the operation succeded */
		if (resp.isNull()) {
			callback(false, {});
			return;
		}
		json::ArrReader<std::u8string_view> list = resp.arr();

		/* read all of the stats */
		std::vector<env::FileStats> stats;
		for (const auto& entry : list)
			stats.push_back(fParseStats(entry.obj()));
		callback(true, stats);
		});
}
void env::FileSystem::createDir(std::u8string_view path, std::function<void(bool)> callback) {
	logger.debug(u8"Creating directory [", path, u8']');
	std::u8string actual = fPrepare(path);
	fHandleTask(str::u8::Build(u8"mkdir:", actual), callback);
}
void env::FileSystem::deleteDir(std::u8string_view path, std::function<void(bool)> callback) {
	logger.debug(u8"Removing directory [", path, u8']');
	std::u8string actual = fPrepare(path);
	fHandleTask(str::u8::Build(u8"rmdir:", actual), callback);
}
void env::FileSystem::deleteFile(std::u8string_view path, std::function<void(bool)> callback) {
	logger.debug(u8"Removing file [", path, u8']');
	std::u8string actual = fPrepare(path);
	fHandleTask(str::u8::Build(u8"rmfile:", actual), callback);
}

void env::FileSystem::openFile(std::u8string_view path, env::FileOpen open, std::function<void(bool, uint64_t, const env::FileStats*)> callback) {
	logger.debug(u8"Opening file [", path, u8']');
	std::u8string actual = fPrepare(path);

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

	/* queue the task to open the file */
	fHandleTask(task, [=, this](json::Reader<std::u8string_view> resp) {
		bool hasId = false, hasStats = false;
		uint64_t id = 0;
		env::FileStats stats;

		/* read the stats and id */
		for (const auto& [key, value] : resp.obj()) {
			if (key == L"id" && !value.isNull()) {
				id = value.unum();
				hasId = true;
			}
			else if (key == L"stats" && !value.isNull()) {
				stats = fParseStats(value.obj());
				hasStats = true;
			}
		}

		/* check if the call itself succeded and register the id */
		if (hasId) {
			/* lookup the id-slot to be used */
			uint64_t slot = 0;
			while (slot < pOpen.size() && pOpen[slot].has_value())
				++slot;
			if (slot >= pOpen.size())
				pOpen.push_back(std::nullopt);

			/* register the id and notify the callback */
			pOpen[slot] = id;
			callback(true, slot, &stats);
			return;
		}

		/* notify the callback about the failed open */
		callback(false, 0, (hasStats ? &stats : 0));
		});
}
void env::FileSystem::readFile(uint64_t id, uint64_t offset, void* data, uint64_t size, std::function<void(uint64_t)> callback) {
	if (fCheck(id))
		fHandleTask(str::u8::Build(u8"read:", pOpen[id].value(), u8":0x", data, u8':', str::As{ U"#x", offset }, u8':', str::As{ U"#x", size }), [=](json::Reader<std::u8string_view> resp) {
		callback(resp.unum());
			});
}
void env::FileSystem::writeFile(uint64_t id, uint64_t offset, const void* data, uint64_t size, std::function<void(bool)> callback) {
	if (fCheck(id))
		fHandleTask(str::u8::Build(u8"write:", pOpen[id].value(), u8":0x", data, u8':', str::As{ U"#x", offset }, u8':', str::As{ U"#x", size }), callback);
}
void env::FileSystem::truncateFile(uint64_t id, uint64_t size, std::function<void(bool)> callback) {
	if (fCheck(id))
		fHandleTask(str::u8::Build(u8"trunc:", pOpen[id].value(), u8':', str::As{ U"#x", size }), callback);
}
void env::FileSystem::closeFile(uint64_t id) {
	if (fCheck(id))
		fCloseFile(pOpen[id].value());
}
bool env::FileSystem::checkFile(uint64_t id) const {
	return fCheck(id);
}
