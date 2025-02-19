#include "../system.h"

static util::Logger logger{ u8"sys::syscall" };

bool sys::detail::FileIO::fCheckFd(int64_t fd) const {
	/* instance can never be larger than pInstance, as instances are never reduced in size */
	if (fd < 0 || fd >= int64_t(pOpen.size()) || pInstance[pOpen[fd].instance].node.get() == 0)
		return false;
	return true;
}
int64_t sys::detail::FileIO::fCheckRead(int64_t fd) const {
	if (!fCheckFd(fd) || !pInstance[pOpen[fd].instance].config.read)
		return errCode::eBadFd;
	if (pInstance[pOpen[fd].instance].type == env::FileType::directory)
		return errCode::eIsDirectory;
	return 0;
}
int64_t sys::detail::FileIO::fCheckWrite(int64_t fd) const {
	if (!fCheckFd(fd) || !pInstance[pOpen[fd].instance].config.write)
		return errCode::eBadFd;
	return 0;
}
bool sys::detail::FileIO::fCheckAccess(const env::FileStats& stats, bool read, bool write, bool execute, bool effIds) const {
	uint32_t uid = 0, gid = 0;

	/* fetch the ids to be used */
	if (effIds) {
		uid = pSyscall->process().euid;
		gid = pSyscall->process().egid;
	}
	else {
		uid = pSyscall->process().uid;
		gid = pSyscall->process().gid;
	}

	/* check for root-privileges */
	if (uid == 0)
		return true;

	/* check for reading-permissions */
	if (read && !stats.access.permissions.rOther && (stats.access.owner != uid || !stats.access.permissions.rOwner) &&
		(stats.access.group != gid || !stats.access.permissions.rGroup))
		return false;

	/* check for writing-permissions */
	if (write && !stats.access.permissions.wOther && (stats.access.owner != uid || !stats.access.permissions.wOwner) &&
		(stats.access.group != gid || !stats.access.permissions.wGroup))
		return false;

	/* check for executing-permissions */
	if (execute && !stats.access.permissions.xOther && (stats.access.owner != uid || !stats.access.permissions.xOwner) &&
		(stats.access.group != gid || !stats.access.permissions.xGroup))
		return false;
	return true;
}
std::pair<std::u8string, int64_t> sys::detail::FileIO::fCheckPath(int64_t dirfd, std::u8string_view path, bool allowEmpty) {
	/* check if the empty-path is allowed to be used */
	if (allowEmpty && path.empty()) {
		/* extract the path of the file-descriptor */
		if (dirfd == consts::fdWDirectory)
			return { pSyscall->process().workingDirectory, errCode::eSuccess };
		else if (!fCheckFd(dirfd))
			return { u8"", errCode::eBadFd };
		return { pInstance[pOpen[dirfd].instance].path, errCode::eSuccess };
	}

	/* validate the path */
	util::PathState state = util::TestPath(path);
	if (state == util::PathState::invalid)
		return { u8"", errCode::eInvalid };

	/* validate the directory-descriptor */
	Instance* instance = 0;
	if (state == util::PathState::relative && dirfd != consts::fdWDirectory) {
		if (!fCheckFd(dirfd))
			return { u8"", errCode::eBadFd };
		instance = &pInstance[pOpen[dirfd].instance];
		if (instance->type != env::FileType::directory)
			return { u8"", errCode::eNotDirectory };
	}

	/* construct the actual path */
	return { util::MergePaths(instance != 0 ? instance->path : pSyscall->process().workingDirectory, path), errCode::eSuccess };
}

int64_t sys::detail::FileIO::fResolveNode(const std::u8string& path, std::function<int64_t(int64_t, const std::u8string&, detail::SharedNode, const env::FileStats&, bool)> callback) {
	pResolve.callback = callback;

	/* check if the maximum number of link expansions has been reached */
	if (++pResolve.linkFollow >= detail::MaxFollowSymLinks) {
		logger.error(u8"Link expansion limit for [", path, u8"] reached");
		return pResolve.callback(errCode::eLoop, path, {}, {}, false);
	}

	/* fetch the stats of the current node and resolve the path further */
	return pRoot->stats([this, path](const env::FileStats* stats) -> int64_t {
		/* root stats can never be null */
		return fResolveNext(u8"/", path, pRoot, *stats);
		});
}
int64_t sys::detail::FileIO::fResolveNext(const std::u8string& path, std::u8string_view lookup, detail::SharedNode node, const env::FileStats& stats) {
	/* split the path up to check if the node needs to be entered further */
	auto [name, remainder] = util::SplitRoot(lookup);

	/* check if the node is a link to be followed */
	if (stats.type == env::FileType::link && (pResolve.follow || !name.empty())) {
		/* check if the permissions allow for the link to be read */
		if (!fCheckAccess(stats, true, false, false, pResolve.effectiveIds)) {
			logger.error(u8"Access to link [", path, u8"] denied");
			return pResolve.callback(errCode::eAccess, path, {}, {}, false);
		}

		/* construct the actual new absolute path to be resolved and resolve it */
		std::u8string base = util::MergePaths(util::SplitName(path).first, stats.link);
		base.append(lookup);
		return fResolveNode(base, pResolve.callback);
	}

	/* check if the end has been reached */
	if (name.empty())
		return pResolve.callback(errCode::eSuccess, path, node, stats, true);

	/* check if the node is a directory */
	if (stats.type != env::FileType::directory) {
		logger.error(u8"Path [", path, u8"] is not a directory");
		return pResolve.callback(errCode::eNotDirectory, path, {}, {}, false);
	}

	/* check if the permissions allow for the directory to be traversed */
	if (!fCheckAccess(stats, false, false, true, pResolve.effectiveIds)) {
		logger.error(u8"Access to directory [", path, u8"] denied");
		return pResolve.callback(errCode::eAccess, path, {}, {}, false);
	}

	/* check if the node is not mounted, in which case a normal lookup needs to be performed */
	std::u8string _name = std::u8string{ name }, _remainder = std::u8string{ remainder }, _path = util::MergePaths(path, _name);
	auto it = pMounted.find(_path);
	if (it == pMounted.end())
		return fResolveLookup(node, _name, _path, _remainder, stats);

	/* query the mount-stats and check if its still valid */
	return it->second->stats([this, _path, _remainder, it, node, _name, stats](const env::FileStats* mstats) -> int64_t {
		/* check if the mount is still valid */
		if (mstats != 0)
			return fResolveNext(_path, _remainder, it->second, *mstats);

		/* remove the mount and perform a normal lookup */
		pMounted.erase(it);
		return fResolveLookup(node, _name, _path, _remainder, stats);
		});
}
int64_t sys::detail::FileIO::fResolveLookup(detail::SharedNode node, const std::u8string& name, const std::u8string& path, const std::u8string& remainder, const env::FileStats& stats) {
	/* Important: ensure node is captured by the lambda, in order to prevent it from going out of scope and being removed before the internal
	*	deferred callback can be invoked, as the internal callback might only capture 'this', but not the reference-counted node itself */
	return node->lookup(name, path, [this, path, remainder, node, stats](detail::SharedNode cnode, const env::FileStats& cstats) -> int64_t {
		/* check if the node was found */
		if (cnode.get() != 0)
			return fResolveNext(path, remainder, cnode, cstats);

		/* check if this was the last entry */
		if (remainder.empty() && !pResolve.findExisting)
			return pResolve.callback(errCode::eSuccess, path, node, stats, false);
		logger.error(u8"Path [", path, u8"] does not exit");
		return pResolve.callback(errCode::eNoEntry, path, {}, {}, false);
		});
}

int64_t sys::detail::FileIO::fLookupNextFd(uint64_t start, bool canFail) {
	/* check if the index lies above all current indices */
	if (start >= pOpen.size()) {
		pOpen.insert(pOpen.end(), (start + 1 - pOpen.size()), {});
		return start;
	}

	/* lookup the new open entry for the node */
	while (start < pOpen.size() && pOpen[start].used)
		++start;

	/* check if a new entry needs to be appended */
	if (start == pOpen.size()) {
		if (canFail && pOpen.size() >= detail::MaxFileDescriptors)
			return -1;
		pOpen.emplace_back();
	}
	return start;
}
int64_t sys::detail::FileIO::fSetupFile(detail::SharedNode node, std::u8string_view path, env::FileType type, const FileIO::InstanceConfig& config, bool closeOnExecute) {
	/* lookup the new instance for the node */
	size_t instance = 0;
	while (instance < pInstance.size() && pInstance[instance].node.get() != 0)
		++instance;
	if (instance == pInstance.size())
		pInstance.emplace_back();

	/* configure the new instance */
	pInstance[instance].node = node;
	pInstance[instance].path = std::u8string{ path };
	pInstance[instance].user = 1;
	pInstance[instance].type = type;
	pInstance[instance].config = config;

	/* lookup the new open entry for the node */
	int64_t index = fLookupNextFd(0, false);

	/* configure the new opened file and update the open-counter */
	pOpen[index].instance = instance;
	pOpen[index].closeOnExecute = closeOnExecute;
	pOpen[index].used = true;
	++pOpened;
	return index;
}
sys::linux::FileStats sys::detail::FileIO::fBuildLinuxStats(const env::FileStats& stats) const {
	linux::FileStats out;
	uint64_t ctime = std::max<uint64_t>(stats.timeAccessedUS, stats.timeModifiedUS);

	/*
	*	no support for hard-links
	*	virtual-device: 0x0345
	*	real-device: 0x0123
	*	rdev: (major: 0xabc, minor: 0x456))
	*/
	out.dev = (stats.virtualized ? 0x0123 : 0x0345);
	out.inode = stats.id;
	out.mode = stats.access.permissions.all;
	out.nlinks = 1;
	out.uid = stats.access.owner;
	out.gid = stats.access.group;
	out.rdev = 0xabc0'0456;
	out.size = stats.size;
	out.blockSize = 512;
	out.blockCount = (out.size + out.blockSize - 1) / out.blockSize;
	out.atime.sec = (stats.timeAccessedUS / 1000'000);
	out.atime.nsec = (stats.timeAccessedUS % 1000'000) * 1000;
	out.mtime.sec = (stats.timeModifiedUS / 1000'000);
	out.mtime.nsec = (stats.timeModifiedUS % 1000'000) * 1000;
	out.ctime.sec = (ctime / 1000'000);
	out.ctime.nsec = (ctime % 1000'000) * 1000;

	/* write the type to the mode */
	switch (stats.type) {
	case env::FileType::directory:
		out.mode |= uint32_t(linux::FileMode::directory);
		break;
	case env::FileType::file:
		out.mode |= uint32_t(linux::FileMode::regular);
		break;
	case env::FileType::link:
		out.mode |= uint32_t(linux::FileMode::link);
		break;
	case env::FileType::pipe:
		out.mode |= uint32_t(linux::FileMode::fifoPipe);
		break;
	case env::FileType::tty:
		out.mode |= uint32_t(linux::FileMode::charDevice);
		break;
	default:
		logger.fatal(u8"Unsupported stat type [", size_t(stats.type), u8"] encountered");
	}
	return out;
}
int64_t sys::detail::FileIO::fRead(size_t instance, uint64_t offset, std::function<int64_t(int64_t)> callback) {
	return pInstance[instance].node->read(offset, pBuffer, callback);
}
int64_t sys::detail::FileIO::fWrite(size_t instance, uint64_t offset) const {
	if (pInstance[instance].config.append)
		logger.fatal(u8"Appending to file currently not supported");
	return pInstance[instance].node->write(offset, pBuffer, [](int64_t result) -> int64_t { return result; });
}

int64_t sys::detail::FileIO::fOpenAt(int64_t dirfd, std::u8string_view path, uint64_t flags, uint64_t mode) {
	/* check if all flags are supported */
	if ((flags & ~consts::openFlagMask) != 0)
		logger.fatal(u8"Unknown file-flags used on file [", path, u8']');

	/* validate the flags */
	if (!detail::IsSet(flags, consts::opReadOnly) && !detail::IsSet(flags, consts::opWriteOnly) && !detail::IsSet(flags, consts::opReadWrite))
		return errCode::eInvalid;
	if (detail::IsSet(flags, consts::opReadWrite) && detail::IsSet(flags, consts::opWriteOnly))
		return errCode::eInvalid;

	/* setup the instance-config */
	FileIO::InstanceConfig config;

	/* extract all of the flag bits */
	config.read = detail::IsSet(flags, consts::opReadWrite) || !detail::IsSet(flags, consts::opWriteOnly);
	config.write = detail::IsSet(flags, consts::opReadWrite) || detail::IsSet(flags, consts::opWriteOnly);
	config.modify = !detail::IsSet(flags, consts::opOpenOnly);
	config.append = detail::IsSet(flags, consts::opAppend);
	config.creation.noctty = detail::IsSet(flags, consts::opNoCTTY);
	config.creation.create = detail::IsSet(flags, consts::opCreate);
	config.creation.exclusive = detail::IsSet(flags, consts::opExclusive);
	config.creation.truncate = detail::IsSet(flags, consts::opTruncate);
	bool follow = !detail::IsSet(flags, consts::opNoFollow);
	bool directory = detail::IsSet(flags, consts::opDirectory);
	bool closeOnExecute = detail::IsSet(flags, consts::opCloseOnExecute);
	if (!config.modify)
		config.read = (config.write = false);
	if (config.creation.create && config.creation.exclusive)
		follow = false;
	if (!config.write)
		config.append = false;
	if (directory && (config.creation.create || config.creation.exclusive || config.creation.truncate))
		return errCode::eInvalid;

	/* validate and construct the final path */
	auto [actual, result] = fCheckPath(dirfd, path, false);
	if (result != errCode::eSuccess)
		return result;

	/* validate the open count */
	if (pOpened >= detail::MaxFileDescriptors)
		return errCode::eMaximumFiles;

	/* configure the resolve-operation to be performed */
	pResolve.linkFollow = 0;
	pResolve.follow = follow;
	pResolve.findExisting = !config.creation.create;
	pResolve.effectiveIds = true;

	/* lookup the file-node to the file (ensure that it cannot fail anymore once the file has been created) */
	return fResolveNode(actual, [this, mode, config, closeOnExecute, directory](int64_t result, const std::u8string& path, detail::SharedNode node, const env::FileStats& stats, bool found) -> int64_t {
		if (result != errCode::eSuccess)
			return result;

		/* check if the child does not exist yet (create must be set to true) */
		if (!found) {
			/* check if the parent directory is writable (i.e. the file could be created) */
			if (!fCheckAccess(stats, false, true, false, true)) {
				logger.error(u8"Access denied to create [", path, u8"] in parent directory");
				return errCode::eAccess;
			}

			/* setup the creation-access */
			env::FileAccess access;
			access.permissions.all = (mode & env::fileModeMask);
			access.owner = pSyscall->process().euid;
			access.group = pSyscall->process().egid;

			/* try to create the file
			*	Important: ensure node is captured by the lambda, in order to prevent it from going out of scope and being removed before the internal
			*	deferred callback can be invoked, as the internal callback might only capture 'this', but not the reference-counted node itself */
			return node->create(util::SplitName(path).second, path, access, [this, config, closeOnExecute, path, node](int64_t result, detail::SharedNode cnode) -> int64_t {
				if (result != errCode::eSuccess)
					return result;
				return fSetupFile(cnode, path, env::FileType::file, config, closeOnExecute);
				});
		}

		/* check if the file was expected to be created */
		if (config.creation.exclusive) {
			logger.error(u8"Path [", path, u8"] already exists");
			return errCode::eExists;
		}

		/* check if a symbolic link is being opened */
		if (stats.type == env::FileType::link) {
			if (config.modify) {
				logger.error(u8"Path [", path, u8"] is a symbolic link");
				return errCode::eLoop;
			}
			if (config.write) {
				logger.error(u8"Symbolic link [", path, u8"] cannot be written to");
				return errCode::eInvalid;
			}
		}

		/* check if its a directory and it is being written to or a directory was expected */
		if (stats.type == env::FileType::directory) {
			if (config.write) {
				logger.error(u8"Path [", path, u8"] is a directory");
				return errCode::eIsDirectory;
			}
		}
		else if (directory) {
			logger.error(u8"Path [", path, u8"] is not a directory");
			return errCode::eNotDirectory;
		}

		/* check if the required permissions are granted */
		if (!fCheckAccess(stats, config.read, config.write, false, true)) {
			logger.error(u8"Access to [", path, u8"] denied");
			return errCode::eAccess;
		}

		/* check if the node is a link or directory and mark them as open (do not require explicit opening) */
		if (stats.type == env::FileType::link || stats.type == env::FileType::directory)
			return fSetupFile(node, path, stats.type, config, closeOnExecute);

		/* perform the open-call on the file-node
		*	Important: ensure node is captured by the lambda, in order to prevent it from going out of scope and being removed before the internal
		*	deferred callback can be invoked, as the internal callback might only capture 'this', but not the reference-counted node itself */
		return node->open(config.creation.truncate, [this, node, path, config, closeOnExecute, type = stats.type](int64_t result) -> int64_t {
			if (result != errCode::eSuccess)
				return result;
			return fSetupFile(node, path, type, config, closeOnExecute);
			});
		});
}
int64_t sys::detail::FileIO::fReadLinkAt(int64_t dirfd, std::u8string_view path, env::guest_t address, uint64_t size) {
	if (size == 0)
		return errCode::eInvalid;

	/* validate and construct the final path */
	auto [actual, result] = fCheckPath(dirfd, path, false);
	if (result != errCode::eSuccess)
		return result;

	/* configure the resolve-operation to be performed */
	pResolve.linkFollow = 0;
	pResolve.follow = false;
	pResolve.findExisting = true;
	pResolve.effectiveIds = true;

	/* resolve the node and perform the stat-read */
	return fResolveNode(actual, [this, size, address](int64_t result, const std::u8string& path, detail::SharedNode node, const env::FileStats& stats, bool) -> int64_t {
		if (result != errCode::eSuccess)
			return result;

		/* check if the permissions allow for the link to be read */
		if (!fCheckAccess(stats, true, false, false, true)) {
			logger.error(u8"Permissions prevent link [", path, u8"] from being read");
			return errCode::eAccess;
		}

		/* check if the destination is a symbolic link */
		if (stats.type != env::FileType::link)
			return errCode::eInvalid;

		/* write the link to the guest */
		result = std::min<int64_t>(size, stats.link.size());
		env::Instance()->memory().mwrite(address, stats.link.data(), result, env::Usage::Write);

		/* mark the link as read (ignore errors, as this is only a meta-data update) */
		return node->linkRead([result](bool success) -> int64_t { return result; });
		});
}
int64_t sys::detail::FileIO::fAccessAt(int64_t dirfd, std::u8string_view path, uint64_t mode, uint64_t flags) {
	/* validate and construct the final path */
	auto [actual, result] = fCheckPath(dirfd, path, detail::IsSet(flags, consts::accEmptyPath));
	if (result != errCode::eSuccess)
		return result;

	/* configure the resolve-operation to be performed */
	pResolve.linkFollow = 0;
	pResolve.follow = !detail::IsSet(flags, consts::accNoFollow);
	pResolve.findExisting = true;
	pResolve.effectiveIds = detail::IsSet(flags, consts::accEIds);

	/* resolve the node and perform the stat-read */
	return fResolveNode(actual, [this, mode](int64_t result, const std::u8string& path, detail::SharedNode node, const env::FileStats& stats, bool) -> int64_t {
		if (result != errCode::eSuccess)
			return result;

		/* check if only the existance of the file is to be determined */
		if (mode == consts::accFOk)
			return errCode::eSuccess;

		/* check if the access permissions allow for the given operation */
		if (!fCheckAccess(stats, detail::IsSet(mode, consts::accROk), detail::IsSet(mode, consts::accWOk), detail::IsSet(mode, consts::accXOk), pResolve.effectiveIds))
			return errCode::eAccess;
		return errCode::eSuccess;
		});
}

bool sys::detail::FileIO::setup(detail::Syscall* syscall) {
	pSyscall = syscall;

	/* setup the initial root node and auto-mounted components (mount stats can never fail) */
	env::FileAccess rootDirs = env::FileAccess{ fs::RootOwner, fs::RootGroup, fs::OwnerAllElseRW }, rootAll = { fs::RootOwner, fs::RootGroup, fs::All };
	pRoot = std::make_shared<impl::RootFileNode>(pSyscall, rootDirs);
	pMounted[u8"/dev"] = std::make_shared<impl::EmpyDirectory>(rootDirs);
	pMounted[u8"/proc"] = std::make_shared<impl::ProcDirectory>(pSyscall, rootDirs);
	pMounted[u8"/proc/self"] = std::make_shared<impl::LinkNode>(str::u8::Build(u8"/proc/", pSyscall->process().pid), rootAll);
	pMounted[u8"/dev/tty"] = std::make_shared<impl::Terminal>(pSyscall, env::FileAccess{ fs::RootOwner, fs::RootGroup, fs::ReadWrite });
	pMounted[u8"/dev/stdin"] = std::make_shared<impl::LinkNode>(u8"/proc/self/fd/0", rootAll);
	pMounted[u8"/dev/stdout"] = std::make_shared<impl::LinkNode>(u8"/proc/self/fd/1", rootAll);
	pMounted[u8"/dev/stderr"] = std::make_shared<impl::LinkNode>(u8"/proc/self/fd/2", rootAll);

	/* setup the initial open file-entries to the terminal (stdin/stdout/stderr - terminal creation happens inplace) */
	if (fOpenAt(consts::fdWDirectory, u8"/dev/tty", consts::opReadOnly, 0) != 0) {
		logger.error(u8"Failed to open [/dev/tty] for stdin as fd=0");
		return false;
	}
	if (fOpenAt(consts::fdWDirectory, u8"/dev/tty", consts::opWriteOnly, 0) != 1) {
		logger.error(u8"Failed to open [/dev/tty] for stdout as fd=1");
		return false;
	}
	if (fOpenAt(consts::fdWDirectory, u8"/dev/tty", consts::opWriteOnly, 0) != 2) {
		logger.error(u8"Failed to open [/dev/tty] for stderr as fd=2");
		return false;
	}

	/* validate the current path */
	std::u8string_view workingDirectory = util::SplitName(pSyscall->process().path).first;
	if (util::TestPath(workingDirectory) != util::PathState::absolute) {
		logger.error(u8"Current directory [", workingDirectory, u8"] must be a valid absolute path");
		return false;
	}
	pSyscall->process().workingDirectory = util::CanonicalPath(workingDirectory);
	logger.info(u8"Configured with current working directory [", pSyscall->process().workingDirectory, u8']');

	/* enable the root node to ensure any upcoming calls are performed on the actual filesystem */
	pRoot->enable();
	return true;
}

int64_t sys::detail::FileIO::openat(int64_t dirfd, std::u8string_view path, uint64_t flags, uint64_t mode) {
	return fOpenAt(dirfd, path, flags, mode);
}
int64_t sys::detail::FileIO::open(std::u8string_view path, uint64_t flags, uint64_t mode) {
	return fOpenAt(consts::fdWDirectory, path, flags, mode);
}
int64_t sys::detail::FileIO::close(int64_t fd) {
	if (!fCheckFd(fd))
		return errCode::eBadFd;

	/* close the underlying file object and remove the reference to it */
	pInstance[pOpen[fd].instance].node->close();
	if (--pInstance[pOpen[fd].instance].user == 0)
		pInstance[pOpen[fd].instance].node.reset();

	/* release the open-entry and update the open-counter */
	pOpen[fd].used = false;
	--pOpened;
	return errCode::eSuccess;
}
int64_t sys::detail::FileIO::read(int64_t fd, env::guest_t address, uint64_t size) {
	/* validate the fd and access */
	int64_t res = fCheckRead(fd);
	if (res != 0 || size == 0)
		return res;

	/* fetch the data to be read */
	pBuffer.resize(size);
	return fRead(pOpen[fd].instance, 0, [this, address](int64_t read) -> int64_t {
		if (read <= 0)
			return read;

		/* write the data back to the guest memory */
		env::Instance()->memory().mwrite(address, pBuffer.data(), read, env::Usage::Write);
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
	return fRead(pOpen[fd].instance, 0, [this](int64_t read) -> int64_t {
		if (read <= 0)
			return read;

		/* write the data back to the guest memory */
		int64_t offset = 0;
		for (size_t i = 0; i < pCached.size(); i += 2) {
			size_t actual = std::min<size_t>(pBuffer.size() - offset, pCached[i + 1]);
			env::Instance()->memory().mwrite(pCached[i], pBuffer.data() + offset, actual, env::Usage::Write);

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
	env::Instance()->memory().mread(pBuffer.data(), address, size, env::Usage::Read);

	/* write the data out */
	return fWrite(pOpen[fd].instance, 0);
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
		env::Instance()->memory().mread(pBuffer.data() + total, address, size, env::Usage::Read);
		total += size;
	}

	/* write the data out */
	return fWrite(pOpen[fd].instance, 0);
}
int64_t sys::detail::FileIO::readlinkat(int64_t dirfd, std::u8string_view path, env::guest_t address, uint64_t size) {
	return fReadLinkAt(dirfd, path, address, size);
}
int64_t sys::detail::FileIO::readlink(std::u8string_view path, env::guest_t address, uint64_t size) {
	return fReadLinkAt(consts::fdWDirectory, path, address, size);
}
int64_t sys::detail::FileIO::fstat(int64_t fd, env::guest_t address) {
	/* validate the file descriptor */
	if (!fCheckFd(fd))
		return errCode::eBadFd;

	/* request the stats from the object */
	return pInstance[pOpen[fd].instance].node->stats([this, address, fd](const env::FileStats* stats) -> int64_t {
		/* check if the stats failed */
		if (stats == 0) {
			logger.error(u8"Failed to fetch stats for an open file [", fd, u8']');
			return errCode::eStale;
		}

		/* check if an unexpected change of the underlying object occurred */
		if (stats->type != pInstance[pOpen[fd].instance].type) {
			logger.error(u8"Type of opened file-descriptor has changed [", fd, u8']');
			return errCode::eStale;
		}

		/* construct the linux-stats */
		linux::FileStats lstats = fBuildLinuxStats(*stats);

		/* write them to the guest */
		env::Instance()->memory().mwrite(address, &lstats, sizeof(linux::FileStats), env::Usage::Write);
		return errCode::eSuccess;
		});
}
int64_t sys::detail::FileIO::fstatat(int64_t dirfd, std::u8string_view path, env::guest_t address, int64_t flags) {
	/* validate and construct the final path */
	auto [actual, result] = fCheckPath(dirfd, path, detail::IsSet(flags, consts::accEmptyPath));
	if (result != errCode::eSuccess)
		return result;

	/* configure the resolve-operation to be performed */
	pResolve.linkFollow = 0;
	pResolve.follow = !detail::IsSet(flags, consts::accNoFollow);
	pResolve.findExisting = true;
	pResolve.effectiveIds = true;

	/* resolve the node and perform the stat-read */
	return fResolveNode(actual, [this, address](int64_t result, const std::u8string& path, detail::SharedNode node, const env::FileStats& stats, bool) -> int64_t {
		if (result != errCode::eSuccess)
			return result;

		/* construct the linux-stats */
		linux::FileStats lstats = fBuildLinuxStats(stats);

		/* write them to the guest */
		env::Instance()->memory().mwrite(address, &lstats, sizeof(linux::FileStats), env::Usage::Write);
		return errCode::eSuccess;
		});
}
int64_t sys::detail::FileIO::access(std::u8string_view path, int64_t mode) {
	return fAccessAt(consts::fdWDirectory, path, mode, 0);
}
int64_t sys::detail::FileIO::faccessat(int64_t dirfd, std::u8string_view path, int64_t mode) {
	return fAccessAt(dirfd, path, mode, 0);
}
int64_t sys::detail::FileIO::faccessat2(int64_t dirfd, std::u8string_view path, int64_t mode, int64_t flags) {
	return fAccessAt(dirfd, path, mode, flags);
}
int64_t sys::detail::FileIO::ioctl(int64_t fd, uint64_t cmd, uint64_t arg) {
	/* validate the file descriptor */
	if (!fCheckFd(fd))
		return errCode::eBadFd;
	if (!pInstance[pOpen[fd].instance].config.modify)
		return errCode::eBadFd;

	/* ioctl is currently not implemented for any objects */
	logger.warn(u8"Executing unsupported ioctl syscall");
	return errCode::eNoTTY;
}
int64_t sys::detail::FileIO::fcntl(int64_t fd, uint64_t cmd, uint64_t arg) {
	/* validate the file descriptor */
	if (!fCheckFd(fd))
		return errCode::eBadFd;

	/* check if its the duplication commands */
	if (cmd == consts::fcntlDuplicateFd || cmd == consts::fcntlDuplicateFdCloseExec) {
		if (arg >= detail::MaxFileDescriptors)
			return errCode::eInvalid;

		/* lookup the new fd to be used */
		int64_t index = fLookupNextFd(arg, true);
		if (index < 0)
			return errCode::eMaximumFiles;

		/* duplicate the entry */
		pOpen[index].closeOnExecute = (cmd == consts::fcntlDuplicateFdCloseExec);
		pOpen[index].instance = pOpen[fd].instance;
		pOpen[index].used = true;
		++pInstance[pOpen[index].instance].user;
		++pOpened;
		return index;
	}

	/* check if the fd-flags are to be updated */
	else if (cmd == consts::fcntlGetFdFlags)
		return (pOpen[fd].closeOnExecute ? consts::fdFlagCloseExec : 0);
	else if (cmd == consts::fcntlSetFdFlags) {
		pOpen[fd].closeOnExecute = detail::IsSet(arg, consts::fdFlagCloseExec);
		return errCode::eSuccess;
	}

	/* check if the file-flags are to be updated */
	else if (cmd == consts::fcntlGetFileStatusFlags) {
		const FileIO::InstanceConfig& config = pInstance[pOpen[fd].instance].config;
		return (config.read ? (config.write ? consts::fsReadWrite : consts::fsReadOnly) : consts::fsWriteOnly) |
			(config.creation.create ? consts::fsCreate : 0) | (config.creation.truncate ? consts::fsTruncate : 0) |
			(config.creation.exclusive ? consts::fsExclusive : 0) | (config.creation.noctty ? consts::fsNoCTTY : 0) |
			(config.append ? consts::fsAppend : 0) | (config.modify ? 0 : consts::fsOpenOnly);
	}
	else if (cmd == consts::fcntlSetFileStatusFlags) {
		/* ensure that all flags are supported */
		if ((arg & ~consts::fsMask) != 0)
			logger.fatal(u8"Unknown file-status flags used in fcntl");
		pInstance[pOpen[fd].instance].config.append = detail::IsSet(arg, consts::fsAppend);
		return errCode::eSuccess;
	}

	/* unsupported cmd */
	logger.warn(u8"Unsupported fcntl command encountered [", cmd, u8']');
	return errCode::eInvalid;
}

sys::detail::FdState sys::detail::FileIO::fdCheck(int64_t fd) const {
	detail::FdState state;

	/* validate the index itself */
	if (!fCheckFd(fd))
		return state;
	state.valid = true;

	/* populate the meta-data of the file-descriptor */
	state.instance = pOpen[fd].instance;
	state.read = pInstance[pOpen[fd].instance].config.read;
	state.write = pInstance[pOpen[fd].instance].config.write;
	state.modify = pInstance[pOpen[fd].instance].config.modify;
	state.append = pInstance[pOpen[fd].instance].config.append;
	state.type = pInstance[pOpen[fd].instance].type;
	return state;
}
int64_t sys::detail::FileIO::fdStats(int64_t fd, std::function<int64_t(int64_t, const env::FileStats*)> callback) const {
	if (!fCheckFd(fd))
		return callback(errCode::eBadFd, 0);

	/* request the stats from the object */
	return pInstance[pOpen[fd].instance].node->stats([this, fd, callback](const env::FileStats* stats) -> int64_t {
		/* check if the stats failed */
		if (stats == 0) {
			logger.error(u8"Failed to fetch stats for an open file [", fd, u8']');
			return callback(errCode::eStale, 0);
		}

		/* check if an unexpected change of the underlying object occurred */
		if (stats->type != pInstance[pOpen[fd].instance].type) {
			logger.error(u8"Type of opened file-descriptor has changed [", fd, u8']');
			return errCode::eStale;
		}

		/* notify the callback about the successful stats */
		return callback(errCode::eSuccess, stats);
		});
}
int64_t sys::detail::FileIO::fdRead(int64_t fd, uint64_t offset, uint64_t size, std::function<int64_t(const uint8_t*, uint64_t)> callback) {
	if (!fCheckFd(fd))
		return callback(0, 0);
	if (size == 0)
		return callback(0, 0);

	/* fetch the data to be read */
	pBuffer.resize(size);
	return fRead(pOpen[fd].instance, offset, [this, callback](int64_t read) -> int64_t {
		if (read <= 0)
			return callback(0, 0);
		return callback(pBuffer.data(), read);
		});
}
