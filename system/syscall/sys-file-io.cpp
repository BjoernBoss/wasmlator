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
	actual = util::MergePaths(instance != 0 ? instance->path : pSyscall->config().wDirectory, path);
	return errCode::eSuccess;
}

int64_t sys::detail::FileIO::fResolveNode(const std::u8string& path, bool follow, bool exact, std::function<int64_t(int64_t, const std::u8string&, detail::SharedNode, const env::FileStats*, bool)> callback) {
	/* check if the maximum number of link expansions has been reached */
	if (++pLinkFollow >= detail::MaxFollowSymLinks) {
		logger.error(u8"Link expansion limit for [", path, u8"] reached");
		return callback(errCode::eLoop, path, {}, 0, false);
	}

	/* fetch the stats of the current node and resolve the path further */
	return pRoot->stats([this, path, follow, exact, callback](const env::FileStats* stats) -> int64_t {
		/* stats of root can never be null */
		return fResolveNext(u8"/", path, follow, exact, pRoot, stats, callback);
		});
}
int64_t sys::detail::FileIO::fResolveNext(const std::u8string& path, std::u8string_view lookup, bool follow, bool exact, detail::SharedNode node, const env::FileStats* stats, std::function<int64_t(int64_t, const std::u8string&, detail::SharedNode, const env::FileStats*, bool)> callback) {
	/* check if the node was found */
	if (stats == 0) {
		logger.error(u8"Path [", path, u8"] does not exist");
		return callback(errCode::eNoEntry, path, {}, 0, false);
	}

	/* split the path up to check if the node needs to be entered further */
	auto [name, remainder] = util::SplitRoot(lookup);

	/* check if the node is a link to be followed */
	if (stats->type == env::FileType::link && (follow || !name.empty())) {
		/* check if the permissions allow for the link to be read */
		if (!fCheckAccess(stats, true, false, false)) {
			logger.error(u8"Access to link [", path, u8"] denied");
			return callback(errCode::eAccess, path, {}, 0, false);
		}

		/* construct the actual new absolute path to be resolved and resolve it */
		std::u8string base = util::MergePaths(util::SplitName(path).first, stats->link);
		base.append(lookup);
		return fResolveNode(base, follow, exact, callback);
	}

	/* check if the end has been reached */
	if (name.empty())
		return callback(errCode::eSuccess, path, node, stats, true);

	/* check if the node is a directory */
	if (stats->type != env::FileType::directory) {
		logger.error(u8"Path [", path, u8"] is not a directory");
		return callback(errCode::eNotDirectory, path, {}, 0, false);
	}

	/* check if the permissions allow for the directory to be traversed */
	if (!fCheckAccess(stats, false, false, true)) {
		logger.error(u8"Access to directory [", path, u8"] denied");
		return callback(errCode::eAccess, path, {}, 0, false);
	}

	/* lookup the next child and continue resolving the path */
	std::u8string _name = std::u8string{ name }, _remainder = std::u8string{ remainder }, next = util::MergePaths(path, _name);
	return node->lookup(_name, [this, next, _remainder, follow, exact, callback, node, _stats = env::FileStats(*stats)](detail::SharedNode cnode, const env::FileStats* cstats) -> int64_t {
		/* check if the node was found */
		if (cstats != 0)
			return fResolveNext(next, _remainder, follow, exact, cnode, cstats, callback);

		/* check if this was the last entry */
		if (_remainder.empty() && !exact)
			return callback(errCode::eSuccess, next, node, &_stats, false);
		logger.error(u8"Path [", next, u8"] does not exit");
		return callback(errCode::eNoEntry, next, {}, 0, false);
		});
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

	/* reset the link-following-counter as this now counts as a new file-operation */
	pLinkFollow = 0;

	/* lookup the file-node to the file (ensure that it cannot fail anymore once the file has been created) */
	return fResolveNode(actual, follow, !create, [this, mode, truncate, exclusive, read, write, openOnly, closeOnExecute, directory](int64_t result, const std::u8string& path, detail::SharedNode node, const env::FileStats* stats, bool found) -> int64_t {
		if (result != errCode::eSuccess)
			return result;

		/* setup the creation-config */
		detail::SetupConfig config;
		config.permissions = (mode & fileMode::mask);
		config.owner = pSyscall->config().euid;
		config.group = pSyscall->config().egid;
		config.truncate = truncate;
		config.exclusive = exclusive;

		/* check if the child does not exist yet (create must be set to true) */
		if (!found) {
			/* check if the parent directory is writable (i.e. the file could be created) */
			if (!fCheckAccess(stats, false, true, false)) {
				logger.error(u8"Access denied to create [", path, u8"] in parent directory");
				return errCode::eAccess;
			}

			/* try to create the file */
			return node->create(util::SplitName(path).second, config, [this, read, write, openOnly, closeOnExecute, path](int64_t result, detail::SharedNode cnode) -> int64_t {
				if (result != errCode::eSuccess)
					return result;
				return fSetupFile(cnode, path, false, read, write, !openOnly, closeOnExecute);
				});
		}

		/* check if the file was expected to be created */
		if (exclusive) {
			logger.error(u8"Path [", path, u8"] already exists");
			return errCode::eExists;
		}

		/* check if a symbolic link is being opened */
		if (stats->type == env::FileType::link) {
			if (!openOnly)
				return errCode::eLoop;
			if (write)
				return errCode::eInvalid;
		}

		/* check if its a directory and it is being written to or a directory was expected */
		if (stats->type == env::FileType::directory) {
			if (write)
				return errCode::eIsDirectory;
		}
		else if (directory)
			return errCode::eNotDirectory;

		/* check if the required permissions are granted */
		if (!fCheckAccess(stats, read, write, false))
			return errCode::eAccess;

		/* check if the node is a link or directory and mark them as open (do not require explicit opening) */
		if (stats->type == env::FileType::link || stats->type == env::FileType::directory)
			return fSetupFile(node, path, stats->type == env::FileType::directory, read, write, !openOnly, closeOnExecute);

		/* perform the open-call on the file-node */
		return node->open(config, [this, node, read, write, openOnly, closeOnExecute, path](int64_t result) -> int64_t {
			if (result != errCode::eSuccess)
				return result;
			return fSetupFile(node, path, false, read, write, !openOnly, closeOnExecute);
			});
		});
}
int64_t sys::detail::FileIO::fSetupFile(detail::SharedNode node, const std::u8string& path, bool directory, bool read, bool write, bool modify, bool closeOnExecute) {
	/* lookup the new instance for the node */
	size_t instance = 0;
	while (instance < pInstance.size() && pInstance[instance].node.get() != 0)
		++instance;
	if (instance == pInstance.size())
		pInstance.emplace_back();

	/* configure the new instance */
	pInstance[instance].node = node;
	pInstance[instance].path = path;
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
	if (--pInstance[instance].user == 0)
		pInstance[instance].node.reset();
}

int64_t sys::detail::FileIO::fRead(size_t instance, std::function<int64_t(int64_t)> callback) {
	return pInstance[instance].node->read(pBuffer, callback);
}
int64_t sys::detail::FileIO::fWrite(size_t instance) const {
	return pInstance[instance].node->write(pBuffer, [](int64_t result) -> int64_t { return result; });
}
int64_t sys::detail::FileIO::fReadLinkAt(int64_t dirfd, std::u8string_view path, env::guest_t address, uint64_t size) {
	if (size == 0)
		return errCode::eInvalid;

	/* validate and construct the final path */
	std::u8string actual;
	int64_t result = fCheckPath(dirfd, path, actual);
	if (result != errCode::eSuccess)
		return result;

	/* reset the link-following-counter as this now counts as a new file-operation */
	pLinkFollow = 0;

	/* resolve the node and perform the stat-read */
	return fResolveNode(actual, false, true, [this, size, address](int64_t result, const std::u8string& path, detail::SharedNode node, const env::FileStats* stats, bool) -> int64_t {
		if (result != errCode::eSuccess)
			return result;

		/* check if the permissions allow for the link to be read */
		if (!fCheckAccess(stats, true, false, false)) {
			logger.error(u8"Permissions prevent link [", path, u8"] from being read");
			return errCode::eAccess;
		}

		/* check if the destination is a symbolic link */
		if (stats->type != env::FileType::link)
			return errCode::eInvalid;

		/* mark the link as read */
		node->linkRead();

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
	return fRead(pOpen[fd].instance, [this, address](int64_t read) -> int64_t {
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
	//return pFiles[](const env::FileStats& stats) -> int64_t {
	//	/* populate the stat-structure */
	//	LinuxStats out;
	//
	//	out.size = stats.size;
	//	return errCode::eIO;
	//	});

	return errCode::eUnknown;
}
