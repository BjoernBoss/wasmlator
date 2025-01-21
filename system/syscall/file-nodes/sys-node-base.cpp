#include "../../system.h"

static util::Logger logger{ u8"sys::syscall" };

int64_t sys::detail::FileNode::linkRead(std::function<int64_t(bool)> callback) {
	return callback(false);
}
int64_t sys::detail::FileNode::lookup(std::u8string_view name, const std::u8string& path, std::function<int64_t(std::shared_ptr<detail::FileNode>, const env::FileStats&)> callback) {
	return callback({}, {});
}
int64_t sys::detail::FileNode::create(std::u8string_view name, const std::u8string& path, const detail::SetupConfig& config, std::function<int64_t(int64_t, std::shared_ptr<detail::FileNode>)> callback) {
	return callback(errCode::eReadOnly, {});
}
int64_t sys::detail::FileNode::open(const detail::SetupConfig& config, std::function<int64_t(int64_t)> callback) {
	return callback(errCode::eIO);
}
int64_t sys::detail::FileNode::read(std::vector<uint8_t>& buffer, std::function<int64_t(int64_t)> callback) {
	return callback(errCode::eIO);
}
int64_t sys::detail::FileNode::write(const std::vector<uint8_t>& buffer, std::function<int64_t(int64_t)> callback) {
	return callback(errCode::eIO);
}
void sys::detail::FileNode::close() {}


sys::detail::VirtualFileNode::VirtualFileNode(env::FileAccess access) {
	pLastRead = host::GetStampUS();
	pLastWrite = pLastRead;
	pUniqueId = util::UniqueId();
	pAccess = access;
}
int64_t sys::detail::VirtualFileNode::fLookupNew(const std::u8string& name, std::function<int64_t(std::shared_ptr<detail::FileNode>, const env::FileStats&)> callback) {
	/* create the new node and check if it could be created */
	return virtualLookup(name, [this, callback, name](std::shared_ptr<detail::VirtualFileNode> node) -> int64_t {
		if (node.get() == 0)
			return callback({}, {});

		/* check if the node is valid and query its stats */
		return node->stats([this, node, callback, name](const env::FileStats* stats) -> int64_t {
			if (stats == 0)
				return callback({}, {});

			/* add the node to the cache and return it */
			pCache[name] = node;
			return callback(node, *stats);
			});
		});
}
int64_t sys::detail::VirtualFileNode::fCreateNew(const std::u8string& name, const detail::SetupConfig& config, std::function<int64_t(int64_t, std::shared_ptr<detail::FileNode>)> callback) {
	/* create the new node and check if it could be created */
	return virtualCreate(name, config, [this, callback, name](int64_t result, std::shared_ptr<detail::VirtualFileNode> node) -> int64_t {
		if (result != errCode::eSuccess)
			return callback(result, {});

		/* update the current write-time as the directory has been modified */
		pLastWrite = host::GetStampUS();

		/* add the node to the cache and return it */
		pCache[name] = node;
		return callback(errCode::eSuccess, node);
		});
}
int64_t sys::detail::VirtualFileNode::virtualLookup(std::u8string_view name, std::function<int64_t(std::shared_ptr<detail::VirtualFileNode>)> callback) const {
	return callback({});
}
int64_t sys::detail::VirtualFileNode::virtualCreate(std::u8string_view name, const detail::SetupConfig& config, std::function<int64_t(int64_t, std::shared_ptr<detail::VirtualFileNode>)> callback) {
	return callback(errCode::eReadOnly, {});
}
int64_t sys::detail::VirtualFileNode::virtualRead(std::vector<uint8_t>& buffer, std::function<int64_t(int64_t)> callback) {
	return callback(errCode::eIO);
}
int64_t sys::detail::VirtualFileNode::virtualWrite(const std::vector<uint8_t>& buffer, std::function<int64_t(int64_t)> callback) {
	return callback(errCode::eIO);
}
int64_t sys::detail::VirtualFileNode::stats(std::function<int64_t(const env::FileStats*)> callback) const {
	return virtualStats([this, callback](const env::FileStats* stats) -> int64_t {
		if (stats == 0)
			return callback(0);

		env::FileStats out;
		out.link = stats->link;
		out.size = stats->size;
		out.type = stats->type;
		out.timeAccessedUS = pLastRead;
		out.timeModifiedUS = pLastWrite;
		out.access = pAccess;
		out.virtualized = true;
		out.uniqueId = pUniqueId;
		return callback(&out);
		});
}
int64_t sys::detail::VirtualFileNode::linkRead(std::function<int64_t(bool)> callback) {
	pLastRead = host::GetStampUS();
	return callback(true);
}
int64_t sys::detail::VirtualFileNode::lookup(std::u8string_view name, const std::u8string& path, std::function<int64_t(std::shared_ptr<detail::FileNode>, const env::FileStats&)> callback) {
	const std::u8string& _name = std::u8string{ name };

	/* check if the node has already been cached */
	auto it = pCache.find(_name);
	if (it == pCache.end())
		return fLookupNew(_name, callback);

	/* fetch the stats of the file */
	return it->second->stats([this, _name, it, callback](const env::FileStats* stats) -> int64_t {
		if (stats == 0) {
			pCache.erase(it);
			return fLookupNew(_name, callback);
		}
		return callback(it->second, *stats);
		});
}
int64_t sys::detail::VirtualFileNode::create(std::u8string_view name, const std::u8string& path, const detail::SetupConfig& config, std::function<int64_t(int64_t, std::shared_ptr<detail::FileNode>)> callback) {
	const std::u8string& _name = std::u8string{ name };

	/* check if the node has already been cached */
	auto it = pCache.find(_name);
	if (it == pCache.end())
		return fCreateNew(_name, config, callback);

	/* check if the node may also just be opened */
	if (config.exclusive)
		return callback(errCode::eExists, {});

	/* check if the cache is still valid and open it */
	return it->second->virtualStats([this, _name, it, callback, config](const env::FileStats* stats) -> int64_t {
		if (stats == 0) {
			pCache.erase(it);
			return fCreateNew(_name, config, callback);
		}

		/* open the node */
		return it->second->open(config, [callback, it](int64_t result) -> int64_t {
			return callback(result, (result == errCode::eSuccess ? it->second : detail::SharedNode{}));
			});
		});
}
int64_t sys::detail::VirtualFileNode::read(std::vector<uint8_t>& buffer, std::function<int64_t(int64_t)> callback) {
	return virtualRead(buffer, [this, callback](int64_t result) -> int64_t {
		if (result >= 0)
			pLastRead = host::GetStampUS();
		return callback(result);
		});
}
int64_t sys::detail::VirtualFileNode::write(const std::vector<uint8_t>& buffer, std::function<int64_t(int64_t)> callback) {
	return virtualWrite(buffer, [this, callback](int64_t result) -> int64_t {
		if (result >= 0)
			pLastWrite = host::GetStampUS();
		return callback(result);
		});
}


sys::detail::impl::LinkNode::LinkNode(std::u8string_view link, env::FileAccess access) : VirtualFileNode{ access }, pLink{ link } {}
int64_t sys::detail::impl::LinkNode::virtualStats(std::function<int64_t(const env::FileStats*)> callback) const {
	env::FileStats stats;
	stats.link = pLink;
	stats.size = pLink.size();
	stats.type = env::FileType::link;
	return callback(&stats);
}


sys::detail::impl::EmpyDirectory::EmpyDirectory(env::FileAccess access) : VirtualFileNode{ access } {}
int64_t sys::detail::impl::EmpyDirectory::virtualStats(std::function<int64_t(const env::FileStats*)> callback) const {
	env::FileStats stats;
	stats.type = env::FileType::directory;
	return callback(&stats);
}
