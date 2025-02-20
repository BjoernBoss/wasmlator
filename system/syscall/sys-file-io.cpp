#include "../system.h"

static util::Logger logger{ u8"sys::syscall" };

sys::detail::FileIO::~FileIO() {
	/* release the chain of nodes (as they might have cycles within them: ancestor -> cache -> ancestor...) */
	if (pRoot.get() != 0)
		pRoot->release();
}

const sys::detail::FileIO::Instance& sys::detail::FileIO::fInstance(int64_t fd) const {
	return pInstance[pOpen[fd].instance];
}
sys::detail::FileIO::Instance& sys::detail::FileIO::fInstance(int64_t fd) {
	return pInstance[pOpen[fd].instance];
}
bool sys::detail::FileIO::fCheckFd(int64_t fd) const {
	/* instance can never be larger than pInstance, as instances are never reduced in size */
	if (fd < 0 || fd >= int64_t(pOpen.size()) || fInstance(fd).node.get() == 0)
		return false;
	return true;
}
int64_t sys::detail::FileIO::fCheckRead(int64_t fd) const {
	if (!fCheckFd(fd) || !fInstance(fd).config.read)
		return errCode::eBadFd;
	if (fInstance(fd).node->type() == env::FileType::directory)
		return errCode::eIsDirectory;
	return 0;
}
int64_t sys::detail::FileIO::fCheckWrite(int64_t fd) const {
	if (!fCheckFd(fd) || !fInstance(fd).config.write)
		return errCode::eBadFd;
	return 0;
}
bool sys::detail::FileIO::fCheckAccess(const detail::NodeStats& stats, bool read, bool write, bool execute, bool effIds) const {
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
std::tuple<sys::detail::SharedNode, std::u8string, int64_t> sys::detail::FileIO::fCheckPath(int64_t dirfd, std::u8string_view path, bool allowEmpty) {
	/* check if the empty-path is allowed to be used */
	if (allowEmpty && path.empty()) {
		/* extract the path of the file-descriptor */
		if (dirfd == consts::fdWDirectory)
			return { pRoot, pSyscall->process().workingDirectory, errCode::eSuccess };
		else if (!fCheckFd(dirfd))
			return { pRoot, u8"", errCode::eBadFd };
		return { fInstance(dirfd).node, u8"", errCode::eSuccess };
	}

	/* validate the path */
	util::PathState state = util::TestPath(path);
	if (state == util::PathState::invalid)
		return { pRoot, u8"", errCode::eInvalid };

	/* check if its an absolute path */
	if (state == util::PathState::absolute)
		return { pRoot, util::CleanPath(path), errCode::eSuccess };

	/* check if the path relative to the current directory has been selected */
	if (dirfd == consts::fdWDirectory)
		return { pRoot, util::CleanPath(str::u8::Build(pSyscall->process().workingDirectory, u8'/', path)), errCode::eSuccess };

	/* validate the directory-descriptor */
	if (!fCheckFd(dirfd))
		return { pRoot, u8"", errCode::eBadFd };
	FileIO::Instance& instance = fInstance(dirfd);
	if (instance.node->type() != env::FileType::directory)
		return { pRoot, u8"", errCode::eNotDirectory };
	return { instance.node, util::CleanPath(path), errCode::eSuccess };
}

int64_t sys::detail::FileIO::fResolveNode(const detail::SharedNode& node, const std::u8string& path, std::function<int64_t(int64_t, const detail::SharedNode&, const detail::NodeStats&, bool)> callback) {
	pResolve.callback = callback;
	return fResolveNext(node, path, 0);
}
int64_t sys::detail::FileIO::fResolveNext(const detail::SharedNode& node, std::u8string_view lookup, const detail::NodeStats* stats) {
	/* split the path up to check if the node needs to be entered further */
	auto [name, remainder] = util::SplitRoot(lookup);

	/* check if the current component is just being resolved */
	if (name == u8".")
		return fResolveNext(node, remainder, stats);

	/* check if the ancestor needs to continue */
	if (name == u8"..")
		return fResolveNext(node->ancestor() == 0 ? pRoot : node->ancestor(), remainder, 0);

	/* check if the stats of the node need to be fetched */
	if (stats != 0)
		return fResolveNextStats(node, name, remainder, *stats);

	/* lookup the stats and continue the call */
	std::u8string _name{ name }, _remainder{ remainder };
	return node->stats([this, _remainder, _name, node](const detail::NodeStats& nstats) -> int64_t {
		return fResolveNextStats(node, _name, _remainder, nstats);
		});
}
int64_t sys::detail::FileIO::fResolveNextStats(const detail::SharedNode& node, std::u8string_view name, std::u8string_view remainder, const detail::NodeStats& stats) {
	/* check if the node is a link to be followed */
	if (node->type() == env::FileType::link && (pResolve.follow || !name.empty())) {
		/* check if the permissions allow for the link to be read */
		if (!fCheckAccess(stats, true, false, false, pResolve.effectiveIds)) {
			logger.error(u8"Access to link [", node->buildPath(), u8"] denied");
			return pResolve.callback(errCode::eAccess, {}, {}, false);
		}

		/* check if the maximum number of link expansions has been reached */
		if (++pResolve.linkFollow >= detail::MaxFollowSymLinks) {
			logger.error(u8"Link expansion limit for [", name, u8'/', remainder, u8"] at [", node->buildPath(), u8"] reached");
			return pResolve.callback(errCode::eLoop, {}, {}, false);
		}

		/* construct the new path to be resolved and resolve it */
		std::u8string path = util::CleanPath(str::u8::Build(stats.link, u8'/', name, u8'/', remainder));

		/* extract the next node to be used (ancestor must exist, as root will never be a link-node) */
		detail::SharedNode actual = (util::TestPath(path) == util::PathState::relative ? node : pRoot);
		return fResolveNext(actual, path, 0);
	}

	/* check if the end has been reached */
	if (name.empty())
		return pResolve.callback(errCode::eSuccess, node, stats, true);

	/* check if the node is a directory */
	if (node->type() != env::FileType::directory) {
		logger.error(u8"Path [", node->buildPath(), u8"] is not a directory");
		return pResolve.callback(errCode::eNotDirectory, {}, {}, false);
	}

	/* check if the permissions allow for the directory to be traversed */
	if (!fCheckAccess(stats, false, false, true, pResolve.effectiveIds)) {
		logger.error(u8"Access to directory [", node->buildPath(), u8"] denied");
		return pResolve.callback(errCode::eAccess, {}, {}, false);
	}

	/* perform the lookup of the next node */
	std::u8string _name = std::u8string{ name }, _remainder = std::u8string{ remainder };
	return node->lookup(name, [this, _remainder, _name, node, stats](const detail::SharedNode& cnode, const detail::NodeStats& nstats) -> int64_t {
		/* check if the node was found */
		if (cnode.get() != 0)
			return fResolveNext(cnode, _remainder, &nstats);

		/* check if this was the last entry */
		if (_remainder.empty() && !pResolve.findExisting)
			return pResolve.callback(errCode::eSuccess, node, stats, false);
		logger.error(u8"Name [", _name, u8"] does not exist in [", node->buildPath(), u8']');
		return pResolve.callback(errCode::eNoEntry, {}, {}, false);
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
int64_t sys::detail::FileIO::fSetupFile(const detail::SharedNode& node, const FileIO::InstanceConfig& config, bool closeOnExecute) {
	/* lookup the new instance for the node */
	size_t instance = 0;
	while (instance < pInstance.size() && pInstance[instance].node.get() != 0)
		++instance;
	if (instance == pInstance.size())
		pInstance.emplace_back();

	/* configure the new instance */
	pInstance[instance].node = node;
	pInstance[instance].user = 1;
	pInstance[instance].config = config;
	pInstance[instance].dirCache.clear();
	pInstance[instance].offset = 0;
	pInstance[instance].outdated = false;

	/* lookup the new open entry for the node */
	int64_t index = fLookupNextFd(0, false);

	/* configure the new opened file and update the open-counter */
	pOpen[index].instance = instance;
	pOpen[index].closeOnExecute = closeOnExecute;
	pOpen[index].used = true;
	++pOpened;
	return index;
}
sys::linux::FileStats sys::detail::FileIO::fBuildLinuxStats(const detail::SharedNode& node, const detail::NodeStats& stats) const {
	linux::FileStats out;
	uint64_t ctime = std::max<uint64_t>(stats.timeAccessedUS, stats.timeModifiedUS);

	/*
	*	no support for hard-links
	*	virtual-device: 0x0345
	*	real-device: 0x0123
	*	rdev: (major: 0xabc, minor: 0x456))
	*/
	out.dev = (node->virtualized() ? 0x0123 : 0x0345);
	out.inode = node->id();
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
	switch (node->type()) {
	case env::FileType::directory:
		out.mode |= uint32_t(linux::FileMode::directory);
		break;
	case env::FileType::file:
		out.mode |= uint32_t(linux::FileMode::regular);
		break;
	case env::FileType::link:
		out.mode |= uint32_t(linux::FileMode::link);
		break;
	case env::FileType::character:
		out.mode |= uint32_t(linux::FileMode::charDevice);
		break;
	default:
		logger.fatal(u8"Unsupported stat type [", size_t(node->type()), u8"] encountered");
	}
	return out;
}
int64_t sys::detail::FileIO::fRead(uint64_t fd, std::optional<uint64_t> offset, std::function<int64_t(int64_t)> callback) {
	FileIO::Instance& instance = fInstance(fd);

	/* fetch the offset to be used */
	bool fileOffset = (!offset.has_value() && instance.node->type() == env::FileType::file);
	uint64_t _offset = (fileOffset ? instance.offset : offset.value_or(0));

	/* perform the actual read of the data */
	return instance.node->read(_offset, pBuffer, [&instance, fileOffset, callback](int64_t result) -> int64_t {
		if (result < 0)
			return callback(result);
		if (fileOffset)
			instance.offset += result;

		/* mark the node as read */
		return instance.node->flagRead([result, callback]() -> int64_t { return callback(result); });
		});
}
int64_t sys::detail::FileIO::fWrite(uint64_t fd, std::optional<uint64_t> offset) {
	FileIO::Instance& instance = fInstance(fd);

	/* setup the final write-function to be used */
	std::function<int64_t()> callback = [this, &instance, offset]() -> int64_t {
		/* fetch the offset to be used */
		bool fileOffset = (!offset.has_value() && instance.node->type() == env::FileType::file);
		uint64_t _offset = (fileOffset ? instance.offset : offset.value_or(0));

		/* perform the actual write of the data */
		return instance.node->write(_offset, pBuffer, [&instance, fileOffset](int64_t result) -> int64_t {
			if (result < 0)
				return result;
			if (fileOffset)
				instance.offset += result;

			/* mark the node as written */
			return instance.node->flagWritten([result]() -> int64_t { return result; });
			});
		};

	/* check if the write can just be executed or if the file-end needs to be fetched */
	if (!instance.config.append || instance.node->type() != env::FileType::file || offset.has_value())
		return callback();

	/* fetch the current file-size to be able to update the file-end */
	return instance.node->stats([&instance, callback](const detail::NodeStats& stats) -> int64_t {
		/* update the file-offset and perform the write */
		instance.offset = stats.size;
		return callback();
		});
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
	auto [start, actual, result] = fCheckPath(dirfd, path, false);
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
	std::u8string _name = std::u8string{ util::SplitName(path).second };
	return fResolveNode(start, actual, [this, mode, config, closeOnExecute, directory, _name](int64_t result, const detail::SharedNode& node, const detail::NodeStats& stats, bool found) -> int64_t {
		if (result != errCode::eSuccess)
			return result;

		/* check if the child does not exist yet (create must be set to true) */
		if (!found) {
			/* check if the parent directory is writable (i.e. the file could be created) */
			if (!fCheckAccess(stats, false, true, false, true)) {
				logger.error(u8"Access denied to create [", _name, u8"] in parent directory [", node->buildPath(), u8']');
				return errCode::eAccess;
			}

			/* setup the creation-access */
			env::FileAccess access;
			access.permissions.all = (mode & env::fileModeMask);
			access.owner = pSyscall->process().euid;
			access.group = pSyscall->process().egid;

			/* try to create the file */
			return node->create(_name, access, [this, config, closeOnExecute, node](int64_t result, const detail::SharedNode& cnode) -> int64_t {
				if (result != errCode::eSuccess)
					return result;
				result = fSetupFile(cnode, config, closeOnExecute);

				/* mark the parent node as dirty */
				return node->flagWritten([result]() -> int64_t { return result; });
				});
		}

		/* check if the file was expected to be created */
		if (config.creation.exclusive) {
			logger.error(u8"Path [", node->buildPath(), u8"] already exists");
			return errCode::eExists;
		}

		/* check if a symbolic link is being opened */
		if (node->type() == env::FileType::link) {
			if (config.modify) {
				logger.error(u8"Path [", node->buildPath(), u8"] is a symbolic link");
				return errCode::eLoop;
			}
			if (config.write) {
				logger.error(u8"Symbolic link [", node->buildPath(), u8"] cannot be written to");
				return errCode::eInvalid;
			}
		}

		/* check if its a directory and it is being written to or a directory was expected */
		if (node->type() == env::FileType::directory) {
			if (config.write) {
				logger.error(u8"Path [", node->buildPath(), u8"] is a directory");
				return errCode::eIsDirectory;
			}
		}
		else if (directory) {
			logger.error(u8"Path [", node->buildPath(), u8"] is not a directory");
			return errCode::eNotDirectory;
		}

		/* check if the required permissions are granted */
		if (!fCheckAccess(stats, config.read, config.write, false, true)) {
			logger.error(u8"Access to [", node->buildPath(), u8"] denied");
			return errCode::eAccess;
		}

		/* check if the node is a link or directory and mark them as open (do not require explicit opening) */
		if (node->type() == env::FileType::link || node->type() == env::FileType::directory)
			return fSetupFile(node, config, closeOnExecute);

		/* perform the open-call on the file-node */
		return node->open(config.creation.truncate, [this, node, config, closeOnExecute](int64_t result) -> int64_t {
			if (result != errCode::eSuccess)
				return result;
			result = fSetupFile(node, config, closeOnExecute);

			/* mark the file as written */
			if (!config.creation.truncate)
				return result;
			return node->flagWritten([result]() -> int64_t {return result; });
			});
		});
}
int64_t sys::detail::FileIO::fReadLinkAt(int64_t dirfd, std::u8string_view path, env::guest_t address, uint64_t size) {
	if (size == 0)
		return errCode::eInvalid;

	/* validate and construct the final path */
	auto [start, actual, result] = fCheckPath(dirfd, path, false);
	if (result != errCode::eSuccess)
		return result;

	/* configure the resolve-operation to be performed */
	pResolve.linkFollow = 0;
	pResolve.follow = false;
	pResolve.findExisting = true;
	pResolve.effectiveIds = true;

	/* resolve the node and perform the stat-read */
	return fResolveNode(start, actual, [this, size, address](int64_t result, const detail::SharedNode& node, const detail::NodeStats& stats, bool) -> int64_t {
		if (result != errCode::eSuccess)
			return result;

		/* check if the permissions allow for the link to be read */
		if (!fCheckAccess(stats, true, false, false, true)) {
			logger.error(u8"Permissions prevent link [", node->buildPath(), u8"] from being read");
			return errCode::eAccess;
		}

		/* check if the destination is a symbolic link */
		if (node->type() != env::FileType::link)
			return errCode::eInvalid;

		/* write the link to the guest */
		result = std::min<int64_t>(size, stats.link.size());
		env::Instance()->memory().mwrite(address, stats.link.data(), result, env::Usage::Write);

		/* mark the link as read */
		return node->flagRead([result]() -> int64_t { return result; });
		});
}
int64_t sys::detail::FileIO::fAccessAt(int64_t dirfd, std::u8string_view path, uint64_t mode, uint64_t flags) {
	/* validate and construct the final path */
	auto [node, actual, result] = fCheckPath(dirfd, path, detail::IsSet(flags, consts::accEmptyPath));
	if (result != errCode::eSuccess)
		return result;

	/* configure the resolve-operation to be performed */
	pResolve.linkFollow = 0;
	pResolve.follow = !detail::IsSet(flags, consts::accNoFollow);
	pResolve.findExisting = true;
	pResolve.effectiveIds = detail::IsSet(flags, consts::accEIds);

	/* resolve the node and perform the stat-read */
	return fResolveNode(node, actual, [this, mode](int64_t result, const detail::SharedNode& node, const detail::NodeStats& stats, bool) -> int64_t {
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
	detail::SharedNode _proc = std::make_shared<impl::ProcDirectory>(pSyscall, rootDirs);
	detail::SharedNode _dev = std::make_shared<impl::EmpyDirectory>(rootDirs);
	pRoot->mount(u8"proc", _proc);
	pRoot->mount(u8"dev", _dev);
	_proc->mount(u8"self", std::make_shared<impl::LinkNode>(str::u8::Build(u8"/proc/", pSyscall->process().pid), rootAll));
	_dev->mount(u8"tty", std::make_shared<impl::Terminal>(pSyscall, env::FileAccess{ fs::RootOwner, fs::RootGroup, fs::ReadWrite }));
	_dev->mount(u8"stdin", std::make_shared<impl::LinkNode>(u8"/proc/self/fd/0", rootAll));
	_dev->mount(u8"stdout", std::make_shared<impl::LinkNode>(u8"/proc/self/fd/1", rootAll));
	_dev->mount(u8"stderr", std::make_shared<impl::LinkNode>(u8"/proc/self/fd/2", rootAll));

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
	pSyscall->process().workingDirectory = util::AbsolutePath(workingDirectory);
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

	/* close the underlying file object */
	return fInstance(fd).node->close([this, fd]() -> int64_t {
		/* remove the reference to the node */
		if (--fInstance(fd).user == 0) {
			fInstance(fd).dirCache.clear();
			fInstance(fd).node.reset();
		}

		/* release the open-entry and update the open-counter */
		pOpen[fd].used = false;
		--pOpened;
		return errCode::eSuccess;
		});
}
int64_t sys::detail::FileIO::read(int64_t fd, env::guest_t address, uint64_t size) {
	/* validate the fd and access */
	int64_t res = fCheckRead(fd);
	if (res != 0 || size == 0)
		return res;

	/* fetch the data to be read */
	pBuffer.resize(size);
	return fRead(fd, std::nullopt, [this, address](int64_t read) -> int64_t {
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
	return fRead(fd, std::nullopt, [this](int64_t read) -> int64_t {
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
	return fWrite(fd, std::nullopt);
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
	return fWrite(fd, std::nullopt);
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
	detail::SharedNode node = fInstance(fd).node;

	/* request the stats from the object */
	return node->stats([this, node, address](const detail::NodeStats& stats) -> int64_t {
		/* construct the linux-stats */
		linux::FileStats lstats = fBuildLinuxStats(node, stats);

		/* write them to the guest */
		env::Instance()->memory().mwrite(address, &lstats, sizeof(linux::FileStats), env::Usage::Write);
		return errCode::eSuccess;
		});
}
int64_t sys::detail::FileIO::fstatat(int64_t dirfd, std::u8string_view path, env::guest_t address, int64_t flags) {
	/* validate and construct the final path */
	auto [start, actual, result] = fCheckPath(dirfd, path, detail::IsSet(flags, consts::accEmptyPath));
	if (result != errCode::eSuccess)
		return result;

	/* configure the resolve-operation to be performed */
	pResolve.linkFollow = 0;
	pResolve.follow = !detail::IsSet(flags, consts::accNoFollow);
	pResolve.findExisting = true;
	pResolve.effectiveIds = true;

	/* resolve the node and perform the stat-read */
	return fResolveNode(start, actual, [this, address](int64_t result, const detail::SharedNode& node, const detail::NodeStats& stats, bool) -> int64_t {
		if (result != errCode::eSuccess)
			return result;

		/* construct the linux-stats */
		linux::FileStats lstats = fBuildLinuxStats(node, stats);

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
	if (!fInstance(fd).config.modify)
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
		++fInstance(index).user;
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
		const FileIO::InstanceConfig& config = fInstance(fd).config;
		return (config.read ? (config.write ? consts::fsReadWrite : consts::fsReadOnly) : consts::fsWriteOnly) |
			(config.creation.create ? consts::fsCreate : 0) | (config.creation.truncate ? consts::fsTruncate : 0) |
			(config.creation.exclusive ? consts::fsExclusive : 0) | (config.creation.noctty ? consts::fsNoCTTY : 0) |
			(config.append ? consts::fsAppend : 0) | (config.modify ? 0 : consts::fsOpenOnly);
	}
	else if (cmd == consts::fcntlSetFileStatusFlags) {
		/* ensure that all flags are supported */
		if ((arg & ~consts::fsMask) != 0)
			logger.fatal(u8"Unknown file-status flags used in fcntl");
		fInstance(fd).config.append = detail::IsSet(arg, consts::fsAppend);
		return errCode::eSuccess;
	}

	/* unsupported cmd */
	logger.warn(u8"Unsupported fcntl command encountered [", cmd, u8']');
	return errCode::eInvalid;
}
int64_t sys::detail::FileIO::getdents(int64_t fd, env::guest_t dirent, uint64_t count) {
	/* validate the file descriptor */
	if (!fCheckFd(fd) || !fInstance(fd).config.read)
		return errCode::eBadFd;
	FileIO::Instance& instance = fInstance(fd);
	if (instance.node->type() != env::FileType::directory)
		return errCode::eNotDirectory;

	/* setup the callback to write the result out */
	std::function<int64_t()> callback = [&instance, dirent, count]() -> int64_t {
		if (instance.offset >= instance.dirCache.size())
			return 0;

		/* compute the length of the longest to come name (without null-termination) */
		size_t len = 0;
		for (size_t i = instance.offset; i < instance.dirCache.size(); ++i)
			len = std::max<size_t>(len, instance.dirCache[i].name.size());

		/* allocate the buffer for the dir-entry structure */
		std::unique_ptr<uint8_t[]> _buffer = std::make_unique<uint8_t[]>(sizeof(linux::DirectoryEntry) + len);
		linux::DirectoryEntry* entry = reinterpret_cast<linux::DirectoryEntry*>(_buffer.get());

		/* iterate over the directories and write as many out as possible */
		uint64_t written = 0;
		while (instance.offset < instance.dirCache.size()) {
			const detail::DirEntry& next = instance.dirCache[instance.offset];
			uint64_t total = (offsetof(linux::DirectoryEntry, name) + next.name.size() + 2);

			/* check if the next entry still fits into the buffer and return invalid if not a single entry was written out */
			if (total > std::numeric_limits<uint16_t>::max()) {
				total = count + 1;
				logger.error(u8"Total directory entry does not fit int dentry-length");
			}
			if (written + total > count)
				return (written == 0 ? errCode::eInvalid : written);

			/* prepare the next entry */
			entry->inode = next.id;
			entry->offset = instance.offset++;
			entry->length = uint16_t(total);
			std::copy(next.name.begin(), next.name.end(), entry->name);
			entry->name[entry->length - 2] = 0;

			/* write the type out */
			switch (next.type) {
			case env::FileType::file:
				entry->name[entry->length - 1] = consts::dEntFile;
				break;
			case env::FileType::directory:
				entry->name[entry->length - 1] = consts::dEntDirectory;
				break;
			case env::FileType::link:
				entry->name[entry->length - 1] = consts::dEntLink;
				break;
			case env::FileType::character:
				entry->name[entry->length - 1] = consts::dEntCharacter;
				break;
			case env::FileType::_end:
				entry->name[entry->length - 1] = consts::dEntUnknown;
				break;
			}

			/* write the entry to memory */
			logger.debug(u8"Writing out: [", next.name, u8']');
			env::Instance()->memory().mwrite(dirent + written, _buffer.get(), total, env::Usage::Write);
			written += total;
		}
		return written;
		};

	/* check if the current list can be continued from the cache */
	if (instance.offset > 0 && !instance.outdated)
		return callback();

	/* fetch the new directory state */
	return instance.node->listDir([&instance, callback, fd](int64_t result, const std::vector<detail::DirEntry>& list) -> int64_t {
		/* check if the read failed */
		if (result != errCode::eSuccess) {
			instance.outdated = true;
			logger.error(u8"Failed to list open directory [", fd, u8']');
			return result;
		}

		/* update the cached list and write the state out */
		instance.outdated = false;
		instance.dirCache = list;
		return callback();
		});
}

sys::detail::FdState sys::detail::FileIO::fdCheck(int64_t fd) const {
	detail::FdState state;

	/* validate the index itself */
	if (!fCheckFd(fd))
		return state;
	state.valid = true;

	/* populate the meta-data of the file-descriptor */
	state.instance = pOpen[fd].instance;
	state.read = fInstance(fd).config.read;
	state.write = fInstance(fd).config.write;
	state.modify = fInstance(fd).config.modify;
	state.append = fInstance(fd).config.append;
	state.type = fInstance(fd).node->type();
	return state;
}
int64_t sys::detail::FileIO::fdStats(int64_t fd, std::function<int64_t(int64_t, const env::FileStats&)> callback) const {
	if (!fCheckFd(fd))
		return callback(errCode::eBadFd, {});
	detail::SharedNode node = fInstance(fd).node;

	/* request the stats from the object */
	return node->stats([this, node, callback](const detail::NodeStats& stats) -> int64_t {
		/* construct the file-stats */
		env::FileStats _stats;
		_stats.link = stats.link;
		_stats.timeModifiedUS = stats.timeModifiedUS;
		_stats.timeAccessedUS = stats.timeAccessedUS;
		_stats.size = stats.size;
		_stats.access = stats.access;
		_stats.id = node->id();
		_stats.type = node->type();
		_stats.virtualized = node->virtualized();

		/* notify the callback about the successful stats */
		return callback(errCode::eSuccess, _stats);
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
