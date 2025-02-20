#include "env-filesystem.h"
#include "../process/process-access.h"

static util::Logger logger{ u8"env::filesystem" };

bool env::FileSystem::fHandleTask(const std::u8string& task, std::function<void(json::Reader<std::u8string_view>)> callback) {
	return detail::ProcessAccess::HandleTask(task, [callback](std::u8string_view response, bool) {
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
	uint16_t required = 0b0001'1111'1111;

	/* iterate over the attributes and apply them */
	for (const auto& [key, value] : obj) {
		if (key == L"mtime_us") {
			out.timeModifiedUS = value.unum();
			required &= ~0b0001'0000'0000;
		}
		else if (key == L"atime_us") {
			out.timeAccessedUS = value.unum();
			required &= ~0b0000'1000'0000;
		}
		else if (key == L"size") {
			out.size = value.unum();
			required &= ~0b0000'0100'0000;
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
			required &= ~0b0000'0010'0000;
		}
		else if (key == L"link") {
			out.link = str::u8::To(value.str());
			required &= ~0b0000'0001'0000;
		}
		else if (key == L"owner") {
			out.access.owner = uint32_t(value.unum());
			required &= ~0b0000'0000'1000;
		}
		else if (key == L"group") {
			out.access.group = uint32_t(value.unum());
			required &= ~0b0000'0000'0100;
		}
		else if (key == L"permissions") {
			out.access.permissions.all = uint16_t(value.unum() & env::fileModeMask);
			required &= ~0b0000'0000'0010;
		}
		else if (key == L"id") {
			out.id = value.unum();
			required &= ~0b0000'0000'0001;
		}
	}

	/* mark the system as not-virtualized */
	out.virtualized = false;

	/* check if all values have been received and return the parsed stats */
	if (required != 0)
		logger.fatal(u8"Received incomplete file-stats");
	return out;
}

void env::FileSystem::readStats(std::u8string_view path, std::function<void(const env::FileStats*)> callback) {
	/* ensure that the path is absolute */
	if (util::TestPath(path) != util::PathState::absolute) {
		logger.error(u8"Path [", path, u8"] is not a valid absolute path");
		callback(0);
		return;
	}

	/* canonicalize the path */
	std::u8string actual = util::AbsolutePath(path);
	logger.debug(u8"Reading stats of [", actual, u8']');

	/* queue the task */
	fHandleTask(str::u8::Build(u8"resolve:", actual), [this, callback](json::Reader<std::u8string_view> resp) {
		if (resp.isNull())
			callback(0);
		else {
			env::FileStats stats = fParseStats(resp.obj());
			callback(&stats);
		}
		});
}
void env::FileSystem::readStats(uint64_t id, std::u8string_view name, std::function<void(const env::FileStats*)> callback) {
	logger.debug(u8"Reading stats of [", id, u8':', name, u8"]");
	fHandleTask(str::u8::Build(u8"stats:", id, u8':', name), [this, callback](json::Reader<std::u8string_view> resp) {
		if (resp.isNull())
			callback(0);
		else {
			env::FileStats stats = fParseStats(resp.obj());
			callback(&stats);
		}
		});
}
void env::FileSystem::readPath(uint64_t id, std::function<void(std::u8string_view)> callback) {
	logger.debug(u8"Reading path of [", id, u8']');
	fHandleTask(str::u8::Build(u8"path:", id), [this, callback](json::Reader<std::u8string_view> resp) {
		if (resp.isNull())
			callback(u8"");
		else
			callback(str::u8::To(resp.str()));
		});
}
void env::FileSystem::readDirectory(uint64_t id, std::function<void(const std::map<std::u8string, env::FileStats>*)> callback) {
	logger.debug(u8"Reading directory of [", id, u8']');
	fHandleTask(str::u8::Build(u8"list:", id), [this, callback](json::Reader<std::u8string_view> resp) {
		if (resp.isNull()) {
			callback(0);
			return;
		}

		/* parse all of the stats */
		std::map<std::u8string, env::FileStats> out;
		for (const auto& [key, value] : resp.obj()) {
			/* skip duplicate keys */
			std::u8string _key = str::u8::To(key);
			if (out.contains(_key))
				continue;

			/* parse the value and add it to the output */
			out[_key] = fParseStats(value.obj());
		}
		callback(&out);
		});
}
void env::FileSystem::accessedObject(uint64_t id, std::function<void(bool)> callback) {
	logger.debug(u8"Marking object [", id, u8"] as accessed");
	fHandleTask(str::u8::Build(u8"accessed:", id), [this, callback](json::Reader<std::u8string_view> resp) {
		callback(resp.boolean());
		});
}
void env::FileSystem::changedObject(uint64_t id, std::function<void(bool)> callback) {
	logger.debug(u8"Marking object [", id, u8"] as changed");
	fHandleTask(str::u8::Build(u8"changed:", id), [this, callback](json::Reader<std::u8string_view> resp) {
		callback(resp.boolean());
		});
}
void env::FileSystem::resizeFile(uint64_t id, uint64_t size, std::function<void(bool)> callback) {
	logger.debug(u8"Resizing file [", id, u8"] to [", size, u8']');
	fHandleTask(str::u8::Build(u8"resize:", id, u8':', size), [this, callback](json::Reader<std::u8string_view> resp) {
		callback(resp.boolean());
		});
}
void env::FileSystem::createFile(uint64_t id, std::u8string_view name, env::FileAccess access, std::function<void(std::optional<uint64_t>)> callback) {
	logger.debug(u8"Creating file [", name, u8"] in [", id, u8']');
	fHandleTask(str::u8::Build(u8"create:", id, u8':', access.owner, u8':', access.group, u8':', access.permissions.all, u8':', name), [this, callback](json::Reader<std::u8string_view> resp) {
		if (resp.isNull())
			callback(std::nullopt);
		else
			callback(resp.unum());
		});
}
void env::FileSystem::readFile(uint64_t id, uint64_t offset, void* data, uint64_t size, std::function<void(std::optional<uint64_t>)> callback) {
	logger.debug(u8"Reading file [", id, u8"] range [", str::As{ U"#010x", offset }, u8" - ", str::As{ U"#010x", (offset + size) }, u8']');
	fHandleTask(str::u8::Build(u8"read:", id, u8":0x", data, u8':', str::As{ U"#x", offset }, u8':', str::As{ U"#x", size }), [callback](json::Reader<std::u8string_view> resp) {
		if (resp.isNull())
			callback(std::nullopt);
		else
			callback(resp.unum());
		});
}
void env::FileSystem::writeFile(uint64_t id, uint64_t offset, const void* data, uint64_t size, std::function<void(std::optional<uint64_t>)> callback) {
	logger.debug(u8"Writing file [", id, u8"] range [", str::As{ U"#010x", offset }, u8" - ", str::As{ U"#010x", (offset + size) }, u8']');
	fHandleTask(str::u8::Build(u8"write:", id, u8":0x", data, u8':', str::As{ U"#x", offset }, u8':', str::As{ U"#x", size }), [callback](json::Reader<std::u8string_view> resp) {
		if (resp.isNull())
			callback(std::nullopt);
		else
			callback(resp.unum());
		});
}
