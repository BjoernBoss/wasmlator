#include "../../system.h"

static util::Logger logger{ u8"sys::syscall" };

sys::detail::FileNode::FileNode(uint64_t id, bool virtualized, env::FileType type) : pId{ id }, pVirtualized{ virtualized }, pType{ type } {
	if (pType == env::FileType::_end)
		logger.fatal(u8"Invalid file-type assigned to node");
}

void sys::detail::FileNode::fLinkNode(const detail::SharedNode& node, const std::u8string& name) {
	/* update the ancestor of the node */
	if (node->pAncestor.get() != 0)
		logger.fatal(u8"Mounting already mounted node");
	node->pAncestor = shared_from_this();
	node->pSelf = name;
}
int64_t sys::detail::FileNode::fRecExpandNames(const std::shared_ptr<FileNode::ExpandNamesState>& state) {
	if (state->next >= state->names.size())
		return state->callback(errCode::eSuccess, state->out);

	/* lookup the next node itself */
	return FileNode::lookup(state->names[state->next], [this, state](const detail::SharedNode& node, const detail::NodeStats&) -> int64_t {
		/* check if the node has been found and can be added */
		if (node.get() != 0)
			state->out.push_back({ state->names[state->next], node->id(), node->type() });

		/* perform the next recursive lookup */
		++state->next;
		return fRecExpandNames(state);
		});
}

void sys::detail::FileNode::release() {
	for (const auto& [_, node] : pMounted)
		node->release();
	pMounted.clear();
	for (const auto& [_, node] : pCache)
		node->release();
	pCache.clear();
}
void sys::detail::FileNode::mount(const std::u8string& name, const detail::SharedNode& node) {
	fLinkNode(node, name);
	pMounted[name] = node;
}
const sys::detail::SharedNode& sys::detail::FileNode::ancestor() const {
	return pAncestor;
}
std::u8string sys::detail::FileNode::buildPath() const {
	std::u8string path = (pAncestor.get() == 0 ? u8"" : pAncestor->buildPath());
	if (path.ends_with(u8"/"))
		return str::u8::Build(path, pSelf);
	return str::u8::Build(path, u8'/', pSelf);
}
env::FileType sys::detail::FileNode::type() const {
	return pType;
}
bool sys::detail::FileNode::virtualized() const {
	return pVirtualized;
}
uint64_t sys::detail::FileNode::id() const {
	return pId;
}

int64_t sys::detail::FileNode::makeFind(std::u8string_view name, std::function<int64_t(const detail::SharedNode&)> callback) const {
	return callback({});
}
int64_t sys::detail::FileNode::makeLookup(std::u8string_view name, std::function<int64_t(const detail::SharedNode&, const detail::NodeStats&)> callback) const {
	return makeFind(name, [callback](const detail::SharedNode& node) -> int64_t {
		if (node.get() == 0)
			return callback({}, {});
		return node->stats([callback, node](const detail::NodeStats& stats) -> int64_t { return callback(node, stats); });
		});
}
int64_t sys::detail::FileNode::makeCreate(std::u8string_view name, env::FileAccess access, std::function<int64_t(int64_t, const detail::SharedNode&)> callback) {
	return callback(errCode::eReadOnly, {});
}
int64_t sys::detail::FileNode::makeListNames(std::function<int64_t(int64_t, const std::vector<std::u8string>&)> callback) {
	return callback(errCode::eNotDirectory, {});
}
int64_t sys::detail::FileNode::makeListDir(std::function<int64_t(int64_t, const std::vector<detail::DirEntry>&)> callback) {
	return makeListNames([this, callback](int64_t result, const std::vector<std::u8string>& list) -> int64_t {
		if (result != errCode::eSuccess)
			return callback(result, {});

		/* setup the shared state and lookup all additional data for the entries */
		std::shared_ptr<FileNode::ExpandNamesState> state = std::make_shared<FileNode::ExpandNamesState>();
		state->callback = callback;
		state->names = list;
		state->next = 0;
		return fRecExpandNames(state);
		});
}

int64_t sys::detail::FileNode::lookup(std::u8string_view name, std::function<int64_t(const detail::SharedNode&, const detail::NodeStats&)> callback) {
	std::u8string _name{ name };
	detail::SharedNode found = 0;

	/* check if the node exists in the mounted nodes */
	auto it = pMounted.find(_name);
	if (it != pMounted.end())
		found = it->second;

	/* check if the node exists in the cache */
	else {
		it = pCache.find(_name);
		if (it != pCache.end())
			found = it->second;
	}

	/* check if the node has already been found */
	if (found)
		return found->stats([found, callback](const detail::NodeStats& stats) -> int64_t { return callback(found, stats); });

	/* perform the actual lookup */
	return makeLookup(name, [this, _name, callback](const detail::SharedNode& node, const detail::NodeStats& stats) -> int64_t {
		/* link the nodes together and check if it can be added to the cache */
		if (node.get() != 0) {
			fLinkNode(node, _name);
			if (node->virtualized())
				pCache.insert({ _name, node });
		}
		return callback(node, stats);
		});
}
int64_t sys::detail::FileNode::create(std::u8string_view name, env::FileAccess access, std::function<int64_t(int64_t, const detail::SharedNode&)> callback) {
	std::u8string _name{ name };
	detail::SharedNode found = 0;

	/* check if the node already exists, in which case this must have happened between the lookup and create */
	if (pMounted.contains(_name) || pCache.contains(_name))
		return callback(errCode::eInterrupted, {});

	/* perform the actual create */
	return makeCreate(name, access, [this, _name, callback](int64_t result, const detail::SharedNode& node) -> int64_t {
		/* link the nodes together and check if it can be added to the cache */
		if (node.get() != 0) {
			fLinkNode(node, _name);
			if (node->virtualized())
				pCache.insert({ _name, node });
		}
		return callback(result, node);
		});
}
int64_t sys::detail::FileNode::listDir(std::function<int64_t(int64_t, const std::vector<detail::DirEntry>&)> callback) {
	/* fetch the actual entries */
	return makeListDir([this, callback](int64_t result, const std::vector<detail::DirEntry>& list) -> int64_t {
		if (result != errCode::eSuccess)
			return callback(result, {});
		std::vector<detail::DirEntry> out;
		std::set<std::u8string_view> seen = { u8".", u8".." };

		/* add the loopback and ancestor entries */
		out.emplace_back(detail::DirEntry{ u8".", FileNode::id(), FileNode::type() });
		detail::FileNode* ancestor = (pAncestor.get() != 0 ? pAncestor.get() : this);
		out.emplace_back(detail::DirEntry{ u8"..", ancestor->id(), ancestor->type() });

		/* add all of the mounted nodes */
		for (const auto& [name, node] : pMounted) {
			if (seen.contains(name))
				continue;
			seen.insert(name);
			out.emplace_back(detail::DirEntry{ name, node->id(), node->type() });
		}

		/* add all of the cached nodes */
		for (const auto& [name, node] : pCache) {
			if (seen.contains(name))
				continue;
			seen.insert(name);
			out.emplace_back(detail::DirEntry{ name, node->id(), node->type() });
		}

		/* add the newly seen nodes */
		for (const detail::DirEntry& entry : list) {
			if (seen.contains(entry.name))
				continue;
			seen.insert(entry.name);
			out.emplace_back(entry);
		}
		return callback(errCode::eSuccess, out);
		});

}
int64_t sys::detail::FileNode::open(bool truncate, std::function<int64_t(int64_t)> callback) {
	return callback(errCode::eAccess);
}
int64_t sys::detail::FileNode::read(uint64_t offset, std::vector<uint8_t>& buffer, std::function<int64_t(int64_t)> callback) {
	return callback(errCode::eIO);
}
int64_t sys::detail::FileNode::write(uint64_t offset, const std::vector<uint8_t>& buffer, std::function<int64_t(int64_t)> callback) {
	return callback(errCode::eIO);
}
int64_t sys::detail::FileNode::close(std::function<int64_t()> callback) {
	return callback();
}


sys::detail::RealFileNode::RealFileNode(uint64_t id, env::FileType type) : FileNode{ id, false, type } {}


sys::detail::VirtFileNode::VirtFileNode(env::FileType type, env::FileAccess access) : FileNode{ util::UniqueId(), true, type } {
	pAccess = access;
	pLastRead = host::GetStampUS();
	pLastWrite = host::GetStampUS();
}
int64_t sys::detail::VirtFileNode::makeStats(std::function<int64_t(const std::u8string&, uint64_t)> callback) const {
	return callback(u8"", 0);
}
int64_t sys::detail::VirtFileNode::stats(std::function<int64_t(const detail::NodeStats&)> callback) const {
	return makeStats([this, callback](const std::u8string& link, uint64_t size) -> int64_t {
		detail::NodeStats stats;
		stats.access = pAccess;
		stats.timeAccessedUS = pLastRead;
		stats.timeModifiedUS = pLastWrite;
		if (FileNode::type() == env::FileType::file)
			stats.size = size;
		else if (FileNode::type() == env::FileType::link) {
			stats.link = link;
			stats.size = size;
		}
		return callback(stats);
		});
}
int64_t sys::detail::VirtFileNode::flagRead(std::function<int64_t()> callback) {
	pLastRead = host::GetStampUS();
	return callback();
}
int64_t sys::detail::VirtFileNode::flagWritten(std::function<int64_t()> callback) {
	pLastWrite = host::GetStampUS();
	return callback();
}


sys::detail::impl::LinkNode::LinkNode(std::u8string_view link, env::FileAccess access) : VirtFileNode{ env::FileType::link, access }, pLink{ link } {}
int64_t sys::detail::impl::LinkNode::makeStats(std::function<int64_t(const std::u8string&, uint64_t)> callback) const {
	return callback(pLink, pLink.size());
}


sys::detail::impl::EmpyDirectory::EmpyDirectory(env::FileAccess access) : VirtFileNode{ env::FileType::directory, access } {}
int64_t sys::detail::impl::EmpyDirectory::makeListNames(std::function<int64_t(int64_t, const std::vector<std::u8string>&)> callback) {
	return callback(errCode::eSuccess, {});
}
