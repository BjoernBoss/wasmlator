#include "../system.h"

static util::Logger logger{ u8"sys::syscall" };

bool sys::detail::FileIO::fCheckFd(int64_t fd) const {
	/* instance can never be larger than pInstance, as instances are never reduced in size */
	if (fd < 0 || fd >= int64_t(pOpen.size()) || pInstance[pOpen[fd].instance].node.get() == 0)
		return false;
	return true;
}
int64_t sys::detail::FileIO::fCheckRead(int64_t fd) const {
	if (!fCheckFd(fd) || !pOpen[fd].read)
		return errCode::eBadFd;
	if (pInstance[pOpen[fd].instance].directory)
		return errCode::eIsDirectory;
	return 0;
}
int64_t sys::detail::FileIO::fCheckWrite(int64_t fd) const {
	if (!fCheckFd(fd) || !pOpen[fd].write)
		return errCode::eBadFd;
	if (pInstance[pOpen[fd].instance].directory)
		return errCode::eIsDirectory;
	return 0;
}
bool sys::detail::FileIO::fCheckAccess(const env::FileStats* stats, bool read, bool write, bool execute) const {
	uint32_t euid = pSyscall->config().euid, egid = pSyscall->config().egid;

	/* check for root-privileges */
	if (euid == 0)
		return true;

	/* check for reading-permissions */
	if (read && !stats->permissions.rOther && (stats->owner != euid || !stats->permissions.rOwner) &&
		(stats->group != egid || !stats->permissions.rGroup))
		return false;

	/* check for writing-permissions */
	if (write && !stats->permissions.wOther && (stats->owner != euid || !stats->permissions.wOwner) &&
		(stats->group != egid || !stats->permissions.wGroup))
		return false;

	/* check for executing-permissions */
	if (execute && !stats->permissions.xOther && (stats->owner != euid || !stats->permissions.xOwner) &&
		(stats->group != egid || !stats->permissions.xGroup))
		return false;
	return true;
}

int64_t sys::detail::FileIO::fResolveNode(std::shared_ptr<detail::FileNode> node, std::u8string_view lookup, bool follow, bool create, bool ancestorWritable, std::function<int64_t(int64_t, std::shared_ptr<detail::FileNode>, const env::FileStats*)> callback) {
	/* split the path up to check if the node needs to be entered further */
	auto [name, remainder] = util::SplitRoot(lookup);

	/* fetch the stats of the current node (for the root-node, ancestorWritable will
	*	always be false, but its stats should exist at all times - as its the root) */
	return node->stats([=, this](const env::FileStats* stats) -> int64_t {
		/* check if the node is invalid/does not exist and remove it */
		if (stats == 0) {
			/* check if the lookup failed */
			if (!name.empty() || !create) {
				fDetachNode(node);
				logger.error(u8"Path [", node->path(), u8"] does not exist");
				return callback(errCode::eNoEntry, {}, 0);
			}

			/* check if the directory can potentially be created */
			if (!ancestorWritable) {
				fDetachNode(node);
				logger.error(u8"Access denied to create [", node->path(), u8"] in parent directory");
				return callback(errCode::eAccess, {}, 0);
			}

			/* return success to indicate that the node can be created (at least regarding the permissions up to this point) */
			return callback(errCode::eSuccess, node, 0);
		}

		/* check if the end has been reached (only if its not a link or the link should not be followed) */
		if (name.empty() && (!follow || stats->type != env::FileType::link))
			return callback(errCode::eSuccess, node, stats);

		/* check if its a link */
		if (stats->type == env::FileType::link) {
			/* check if the permissions allow for the link to be read */
			if (!fCheckAccess(stats, true, false, false)) {
				logger.error(u8"Access to link [", node->path(), u8"] denied");
				return callback(errCode::eAccess, {}, 0);
			}

			/* check if the maximum number of link expansions has been reached */
			if (++pLinkFollow >= detail::MaxFollowSymLinks) {
				logger.error(u8"Link expansion limit at [", node->path(), u8"] reached");
				return callback(errCode::eLoop, {}, 0);
			}

			/* construct the actual new absolute path to be resolved */
			std::u8string base = util::MergePaths(util::SplitName(node->path()).first, stats->link);
			base = util::MergePaths(base, lookup);

			/* resolve the new node, based on the expanded link */
			return fResolveNode(pRoot, base, follow, create, false, callback);
		}

		/* check if the node is a directory */
		if (stats->type != env::FileType::directory) {
			logger.error(u8"Path [", node->path(), u8"] is not a directory");
			return callback(errCode::eNotDirectory, {}, 0);
		}

		/* check if the permissions allow for the directory to be traversed */
		if (!fCheckAccess(stats, false, false, true)) {
			logger.error(u8"Access to directory [", node->path(), u8"] denied");
			return callback(errCode::eAccess, {}, 0);
		}

		/* check if the child-node already exists and otherwise fetch the new child-node and register it */
		std::u8string path = util::MergePaths(node->path(), name);
		auto it = pMap.find(path);
		if (it == pMap.end())
			it = pMap.insert({ path, node->spawn(path, name) }).first;

		/* check if the current directory is writable (necessary to determine if child can be created) and resolve the remaining path */
		bool dirWritable = fCheckAccess(stats, false, true, false);
		return fResolveNode(it->second, remainder, follow, create, dirWritable, callback);
		});
}
int64_t sys::detail::FileIO::fLookupNode(std::u8string_view path, bool follow, bool create, std::function<int64_t(int64_t, std::shared_ptr<detail::FileNode>, const env::FileStats*)> callback) {
	/* reset the link-following and start resolving the node */
	pLinkFollow = 0;
	return fResolveNode(pRoot, path, follow, create, false, callback);
}
int64_t sys::detail::FileIO::fSetupFile(std::shared_ptr<detail::FileNode> node, uint64_t id, bool directory, bool read, bool write, bool modify, bool closeOnExecute) {
	/* lookup the new instance for the node */
	size_t instance = 0;
	while (instance < pInstance.size() && pInstance[instance].node.get() != 0)
		++instance;
	if (instance == pInstance.size())
		pInstance.emplace_back();

	/* configure the new instance */
	pInstance[instance].node = node;
	pInstance[instance].id = id;
	pInstance[instance].user = 1;
	pInstance[instance].directory = directory;

	/* lookup the new open entry for the node */
	size_t index = 0;
	while (index < pOpen.size() && pOpen[index].used)
		++index;
	if (index == pOpen.size())
		pOpen.emplace_back();

	/* configure the new opened file */
	pOpen[index].instance = instance;
	pOpen[index].read = read;
	pOpen[index].write = write;
	pOpen[index].modify = modify;
	pOpen[index].closeOnExecute = closeOnExecute;
	pOpen[index].used = true;
	return int64_t(index);
}

void sys::detail::FileIO::fDropInstance(size_t instance) {
	if (--pInstance[instance].user > 0)
		return;
	pInstance[instance].node.reset();
}
void sys::detail::FileIO::fDetachNode(std::shared_ptr<detail::FileNode> node) {
	/* iterate over the map, and erase all true children of the current node */
	for (auto it = pMap.begin(); it != pMap.end();) {
		if (!it->first.starts_with(node->path()))
			++it;

		/* check if its the node itself, or a true child */
		else if (it->first.size() == node->path().size() || it->first[node->path().size()] == u8'/')
			it = pMap.erase(it);
		else
			++it;
	}
}

int64_t sys::detail::FileIO::fCheckPath(int64_t dirfd, std::u8string_view path, std::u8string& actual) {
	/* validate the path */
	util::PathState state = util::TestPath(path);
	if (state == util::PathState::invalid)
		return errCode::eInvalid;

	/* validate the directory-descriptor */
	Instance* instance = 0;
	if (state == util::PathState::relative) {
		if (dirfd != detail::fdWDirectory && !fCheckFd(dirfd))
			return errCode::eBadFd;
		instance = &pInstance[pOpen[dirfd].instance];
		if (!instance->directory)
			return errCode::eNotDirectory;
	}

	/* construct the actual path */
	actual = util::MergePaths(instance != 0 ? instance->node->path() : pSyscall->config().wDirectory, path);
	return errCode::eSuccess;
}
int64_t sys::detail::FileIO::fOpenAt(int64_t dirfd, std::u8string_view path, uint64_t flags, uint64_t mode) {
	/* check if all flags are supported */
	if ((flags & ~fileFlags::mask) != 0)
		logger.fatal(u8"Unknown file-flags used on file [", path, u8']');

	/* validate the flags */
	if ((flags & fileFlags::readOnly) != fileFlags::readOnly
		&& (flags & fileFlags::writeOnly) != fileFlags::writeOnly
		&& (flags & fileFlags::readWrite) != fileFlags::readWrite)
		return errCode::eInvalid;
	if ((flags & fileFlags::writeOnly) == fileFlags::writeOnly && (flags & fileFlags::readWrite) == fileFlags::readWrite)
		return errCode::eInvalid;

	/* extract all of the flag bits */
	bool read = ((flags & fileFlags::readWrite) == fileFlags::readWrite) || ((flags & fileFlags::writeOnly) != fileFlags::writeOnly);
	bool write = ((flags & fileFlags::readWrite) == fileFlags::readWrite) || ((flags & fileFlags::writeOnly) == fileFlags::writeOnly);
	bool follow = ((flags & fileFlags::noSymlinkFollow) != fileFlags::noSymlinkFollow);
	bool directory = ((flags & fileFlags::directory) == fileFlags::directory);
	bool openOnly = ((flags & fileFlags::openOnly) == fileFlags::openOnly);
	bool create = ((flags & fileFlags::create) == fileFlags::create);
	bool exclusive = ((flags & fileFlags::exclusive) == fileFlags::exclusive);
	bool truncate = ((flags & fileFlags::truncate) == fileFlags::truncate);
	bool closeOnExecute = ((flags & fileFlags::closeOnExecute) == fileFlags::closeOnExecute);
	if (openOnly)
		read = (write = false);
	if (create && exclusive)
		follow = false;
	if (directory && (create || exclusive || truncate))
		return errCode::eInvalid;

	/* validate and construct the final path */
	std::u8string actual;
	int64_t result = fCheckPath(dirfd, path, actual);
	if (result != errCode::eSuccess)
		return result;

	/* lookup the file-node to the file (ensure that it cannot fail anymore once a proper file has
	*	been created, as the actual file might otherwise be created, without ever being used) */
	return fLookupNode(actual, follow, create, [=, this](int64_t result, std::shared_ptr<detail::FileNode> node, const env::FileStats* stats) -> int64_t {
		/* check if the lookup failed */
		if (result != errCode::eSuccess)
			return result;

		/* check if the node already exists and validate the properties */
		if (stats != 0) {
			/* check if a symbolic link is being opened */
			if (!follow && stats->type == env::FileType::link && !openOnly)
				return errCode::eLoop;

			/* check if the directory-flag matches */
			if (directory && stats->type != env::FileType::directory)
				return errCode::eNotDirectory;

			/* check if a directory/link is trying to be written */
			if (stats->type == env::FileType::directory && write)
				return errCode::eIsDirectory;
			if (stats->type == env::FileType::link && write)
				return errCode::eInvalid;

			/* check if the required permissions are granted */
			if (!fCheckAccess(stats, read, write, false))
				return errCode::eAccess;

			/* check if the node is a link or directory and mark them as open (do not require explicit opening) */
			if (stats->type == env::FileType::link || stats->type == env::FileType::directory)
				return fSetupFile(node, 0, stats->type == env::FileType::directory, read, write, !openOnly, closeOnExecute);
		}

		/* setup the creation-config */
		detail::SetupConfig config;
		config.permissions = (mode & fileMode::mask);
		config.owner = pSyscall->config().euid;
		config.group = pSyscall->config().egid;
		config.create = create;
		config.truncate = truncate;
		config.open = !exclusive;

		/* try to open (and potentially create) the node */
		return node->open(config, [=, this](int64_t result, uint64_t id) -> int64_t {
			/* check if the open was successful and setup the file-descriptor */
			if (result == errCode::eSuccess)
				return fSetupFile(node, id, false, read, write, !openOnly, closeOnExecute);

			/* check if the open failed for unknown reasons, in which case the node needs to be detached, and a new open tried */
			if (result == errCode::eUnknown) {
				fDetachNode(node);
				return fOpenAt(dirfd, path, flags, mode);
			}

			/* check if the open failed for non-existing reasons, in which case the node can be detached */
			if (result == errCode::eNoEntry || result == errCode::eIsDirectory)
				fDetachNode(node);
			return result;
			});
		});
}
int64_t sys::detail::FileIO::fRead(size_t instance, std::function<int64_t(int64_t)> callback) {
	return pInstance[instance].node->read(pBuffer, callback);
}
int64_t sys::detail::FileIO::fWrite(size_t instance) const {
	return pInstance[instance].node->write(pBuffer, [=](int64_t result) -> int64_t { return result; });
}
int64_t sys::detail::FileIO::fReadLinkAt(int64_t dirfd, std::u8string_view path, env::guest_t address, uint64_t size) {
	if (size == 0)
		return errCode::eInvalid;

	/* validate and construct the final path */
	std::u8string actual;
	int64_t result = fCheckPath(dirfd, path, actual);
	if (result != errCode::eSuccess)
		return result;

	/* resolve the node and perform the stat-read */
	return fLookupNode(actual, false, false, [=, this](int64_t result, std::shared_ptr<detail::FileNode> node, const env::FileStats* stats) -> int64_t {
		/* check if the lookup failed */
		if (stats == 0)
			return result;

		/* check if the permissions allow for the link to be read */
		if (!fCheckAccess(stats, true, false, false)) {
			logger.error(u8"Permissions prevent link [", actual, u8"] from being read");
			return errCode::eAccess;
		}

		/* check if the destination is a symbolic link */
		if (stats->type != env::FileType::link)
			return errCode::eInvalid;

		/* write the link to the guest */
		result = std::min<int64_t>(size, stats->link.size());
		env::Instance()->memory().mwrite(address, stats->link.data(), uint32_t(result), env::Usage::Write);
		return result;
		});
}

bool sys::detail::FileIO::setup(detail::Syscall* syscall) {
	pSyscall = syscall;

	/* setup the initial root node */
	pRoot = std::make_shared<detail::impl::RootFileNode>(pSyscall);
	pMap[u8"/"] = pRoot;

	/* setup the initial open file-entries to the terminal (stdin/stdout/stderr - terminal creation happens inplace) */
	if (fOpenAt(detail::fdWDirectory, u8"/dev/tty", fileFlags::readOnly, 0) != 0) {
		logger.error(u8"Failed to open [/dev/tty] for stdin as fd=0");
		return false;
	}
	if (fOpenAt(detail::fdWDirectory, u8"/dev/tty", fileFlags::writeOnly, 0) != 1) {
		logger.error(u8"Failed to open [/dev/tty] for stdout as fd=1");
		return false;
	}
	if (fOpenAt(detail::fdWDirectory, u8"/dev/tty", fileFlags::writeOnly, 0) != 2) {
		logger.error(u8"Failed to open [/dev/tty] for stderr as fd=2");
		return false;
	}

	/* validate the current path */
	std::u8string_view wDirectory = util::SplitName(pSyscall->config().path).first;
	if (util::TestPath(wDirectory) != util::PathState::absolute) {
		logger.error(u8"Current directory [", wDirectory, u8"] must be a valid absolute path");
		return false;
	}
	pSyscall->config().wDirectory = util::CanonicalPath(wDirectory);
	logger.info(u8"Configured with current working directory [", pSyscall->config().wDirectory, u8']');
	return true;
}

int64_t sys::detail::FileIO::openat(int64_t dirfd, std::u8string_view path, uint64_t flags, uint64_t mode) {
	return fOpenAt(dirfd, path, flags, mode);
}
int64_t sys::detail::FileIO::open(std::u8string_view path, uint64_t flags, uint64_t mode) {
	return fOpenAt(detail::fdWDirectory, path, flags, mode);
}
int64_t sys::detail::FileIO::read(int64_t fd, env::guest_t address, uint64_t size) {
	/* validate the fd and access */
	int64_t res = fCheckRead(fd);
	if (res != 0 || size == 0)
		return res;

	/* fetch the data to be read */
	pBuffer.resize(size);
	return fRead(pOpen[fd].instance, [address, this](int64_t read) -> int64_t {
		if (read <= 0)
			return read;

		/* write the data back to the guest memory */
		env::Instance()->memory().mwrite(address, pBuffer.data(), uint32_t(read), env::Usage::Write);
		return read;
		});
}
int64_t sys::detail::FileIO::readv(int64_t fd, env::guest_t vec, uint64_t count) {
	/* validate the fd and access */
	int64_t res = fCheckRead(fd);
	if (res != 0 || count == 0)
		return res;
	pCached.clear();
	pBuffer.clear();

	/* collect the total readable size */
	for (size_t i = 0; i < count; ++i) {
		env::guest_t address = env::Instance()->memory().read<env::guest_t>(vec + (i * 2 + 0) * sizeof(env::guest_t));
		uint64_t size = env::Instance()->memory().read<env::guest_t>(vec + (i * 2 + 1) * sizeof(env::guest_t));
		if (size == 0)
			continue;
		pCached.push_back(address);
		pCached.push_back(size);
		pBuffer.insert(pBuffer.end(), size, 0);
	}

	/* fetch the data to be read */
	return fRead(pOpen[fd].instance, [this](int64_t read) -> int64_t {
		if (read <= 0)
			return read;

		/* write the data back to the guest memory */
		int64_t offset = 0;
		for (size_t i = 0; i < pCached.size(); i += 2) {
			size_t actual = std::min<size_t>(pBuffer.size() - offset, pCached[i + 1]);
			env::Instance()->memory().mwrite(pCached[i], pBuffer.data() + offset, uint32_t(actual), env::Usage::Write);

			/* check if the totally read data have been written back */
			offset += actual;
			if (offset >= int64_t(pBuffer.size()))
				break;
		}
		return read;
		});
}
int64_t sys::detail::FileIO::write(int64_t fd, env::guest_t address, uint64_t size) {
	/* validate the fd and access */
	int64_t res = fCheckWrite(fd);
	if (res != 0 || size == 0)
		return res;

	/* read the data from the guest */
	pBuffer.resize(size);
	env::Instance()->memory().mread(pBuffer.data(), address, uint32_t(size), env::Usage::Read);

	/* write the data out */
	return fWrite(pOpen[fd].instance);
}
int64_t sys::detail::FileIO::writev(int64_t fd, env::guest_t vec, uint64_t count) {
	/* validate the fd and access */
	int64_t res = fCheckWrite(fd);
	if (res != 0 || count == 0)
		return res;
	pBuffer.clear();
	size_t total = 0;

	/* read the data from the guest */
	for (size_t i = 0; i < count; ++i) {
		/* read the next vector to be written out */
		env::guest_t address = env::Instance()->memory().read<env::guest_t>(vec + (i * 2 + 0) * sizeof(env::guest_t));
		uint64_t size = env::Instance()->memory().read<env::guest_t>(vec + (i * 2 + 1) * sizeof(env::guest_t));
		if (size == 0)
			continue;

		/* collect the data and append them to the buffer */
		pBuffer.insert(pBuffer.end(), size, 0);
		env::Instance()->memory().mread(pBuffer.data() + total, address, uint32_t(size), env::Usage::Read);
		total += size;
	}

	/* write the data out */
	return fWrite(pOpen[fd].instance);
}
int64_t sys::detail::FileIO::readlinkat(int64_t dirfd, std::u8string_view path, env::guest_t address, uint64_t size) {
	return fReadLinkAt(dirfd, path, address, size);
}
int64_t sys::detail::FileIO::readlink(std::u8string_view path, env::guest_t address, uint64_t size) {
	return fReadLinkAt(detail::fdWDirectory, path, address, size);
}
int64_t sys::detail::FileIO::fstat(int64_t fd, env::guest_t address) {
	/* vaidate the file descriptor */
	if (!fCheckFd(fd))
		return errCode::eBadFd;

	/* linux stat-structure */
	struct LinuxStats {
		uint64_t dev = 0;
		uint64_t inode = 0;
		uint32_t mode = 0;
		uint32_t nlinks = 0;
		uint32_t uid = 0;
		uint32_t gid = 0;
		uint64_t rdev = 0;
		int64_t size = 0;
		int32_t blockSize = 0;
		int64_t blockCount = 0;
		int64_t atime = 0;
		int64_t atime_ns = 0;
		int64_t mtime = 0;
		int64_t mtime_ns = 0;
		int64_t ctime = 0;
		int64_t ctime_ns = 0;
	};

	// validate structure!;
	
	logger.fatal(u8"Currently not implemented");

	/* request the stats from the file */
	//return pFiles[fd].node->stats([address, this](const env::FileStats& stats) -> int64_t {
	//	/* populate the stat-structure */
	//	LinuxStats out;
	//
	//	out.size = stats.size;
	//	return errCode::eIO;
	//	});

	return errCode::eUnknown;
}
