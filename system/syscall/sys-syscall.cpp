#include "../system.h"

static util::Logger logger{ u8"sys::Syscall" };

std::u8string sys::detail::Syscall::fReadString(env::guest_t address) const {
	env::Memory& mem = env::Instance()->memory();
	std::u8string out;

	/* read the string from the guest memory */
	for (char8_t c = mem.read<char8_t>(address++); c != 0; c = mem.read<char8_t>(address++))
		out.push_back(c);

	return out;
}

void sys::detail::Syscall::fWrap(std::function<int64_t()> callback) {
	/* execute the wrapped call (to catch any memory exceptions) */
	try {
		pCurrent.result = callback();
	}
	catch (const env::MemoryFault& e) {
		logger.debug(u8"Memory fault at [", str::As{ U"#018x", e.accessed }, u8"] while handling syscall");
		pCurrent.result = errCode::eFault;
	}

	/* if this point is reached, the call has been completed (otherwise await-exception will be thrown) */
	logger.debug(u8"result: ", str::As{ U"#018x", pCurrent.result });
	pUserspace->cpu()->syscallSetResult(pCurrent.result);

	/* check if this is not in-place execution, in which case the execution needs to be
	*	restarted at the next address (pc will already be set by the await-exception) */
	if (!pCurrent.inplace)
		pUserspace->execute();
}
int64_t sys::detail::Syscall::fDispatch() {
	sys::SyscallArgs args = pUserspace->cpu()->syscallGetArgs();

	/* check if the syscall can be handled in-place */
	switch (args.index) {
	case sys::SyscallIndex::getuid:
		return fHandleIds(u8"getuid");
	case sys::SyscallIndex::geteuid:
		return fHandleIds(u8"geteuid");
	case sys::SyscallIndex::getgid:
		return fHandleIds(u8"getgid");
	case sys::SyscallIndex::getegid:
		return fHandleIds(u8"getegid");
	case sys::SyscallIndex::brk: {
		logger.debug(u8"Syscall brk(", str::As{ U"#018x", args.args[0] }, u8')');
		return pMemory.brk(args.args[0]);
	}
	case sys::SyscallIndex::mmap: {
		logger.debug(u8"Syscall mmap(", str::As{ U"#018x", args.args[0] }, u8", ", str::As{ U"#010x", args.args[1] }, u8", ", args.args[2], u8", ", args.args[3], u8", ", args.args[4], u8", ", args.args[5], u8')');
		return pMemory.mmap(args.args[0], args.args[1], uint32_t(args.args[2]), uint32_t(args.args[3]), args.args[4], args.args[5]);
	}
	case sys::SyscallIndex::uname: {
		logger.debug(u8"Syscall uname(", str::As{ U"#018x", args.args[0] }, u8")");
		return fHandleUName(args.args[0]);
	}
	case sys::SyscallIndex::openat: {
		logger.debug(u8"Syscall openat(", args.args[0], u8", ", str::As{ U"#018x", args.args[1] }, u8", ", args.args[2], u8", ", args.args[3], u8')');
		std::u8string path = fReadString(args.args[1]);
		logger.debug(u8"pathname: [", path, u8"]");
		return pFileIO.openat(args.args[0], path, args.args[2], args.args[3]);
	}
	case sys::SyscallIndex::open: {
		logger.debug(u8"Syscall open(", str::As{ U"#018x", args.args[0] }, u8", ", args.args[1], u8", ", args.args[2], u8')');
		std::u8string path = fReadString(args.args[0]);
		logger.debug(u8"pathname: [", path, u8"]");
		return pFileIO.open(path, args.args[1], args.args[2]);
	}
	case sys::SyscallIndex::readv: {
		logger.debug(u8"Syscall readv(", args.args[0], u8", ", str::As{ U"#018x", args.args[1] }, u8", ", args.args[2], u8')');
		return pFileIO.readv(args.args[0], args.args[1], args.args[2]);
	}
	case sys::SyscallIndex::read: {
		logger.debug(u8"Syscall read(", args.args[0], u8", ", str::As{ U"#018x", args.args[1] }, u8", ", args.args[2], u8')');
		return pFileIO.read(args.args[0], args.args[1], args.args[2]);
	}
	case sys::SyscallIndex::writev: {
		logger.debug(u8"Syscall writev(", args.args[0], u8", ", str::As{ U"#018x", args.args[1] }, u8", ", args.args[2], u8')');
		return pFileIO.writev(args.args[0], args.args[1], args.args[2]);
	}
	case sys::SyscallIndex::write: {
		logger.debug(u8"Syscall write(", args.args[0], u8", ", str::As{ U"#018x", args.args[1] }, u8", ", args.args[2], u8')');
		return pFileIO.write(args.args[0], args.args[1], args.args[2]);
	}
	case sys::SyscallIndex::readlinkat: {
		logger.debug(u8"Syscall readlinkat(", args.args[0], u8", ", str::As{ U"#018x", args.args[1] }, u8", ", args.args[2], u8", ", args.args[3], u8')');
		std::u8string path = fReadString(args.args[1]);
		logger.debug(u8"pathname: [", path, u8"]");
		return pFileIO.readlinkat(args.args[0], path, args.args[2], args.args[3]);
	}
	case sys::SyscallIndex::readlink: {
		logger.debug(u8"Syscall readlink(", str::As{ U"#018x", args.args[0] }, u8", ", args.args[1], u8", ", args.args[2], u8')');
		std::u8string path = fReadString(args.args[0]);
		logger.debug(u8"pathname: [", path, u8"]");
		return pFileIO.readlink(path, args.args[1], args.args[2]);
	}
	case sys::SyscallIndex::fstat: {
		logger.debug(u8"Syscall fstat(", args.args[0], u8", ", args.args[1], u8')');
		return pFileIO.fstat(args.args[0], args.args[1]);
	}
	case sys::SyscallIndex::unknown:
		throw detail::UnknownSyscall{ pCurrent.address, args.rawIndex };
		break;
	default:
		logger.fatal(u8"Syscall currently not implemented");
		break;
	}

	/* should never be reached */
	return errCode::eUnknown;
}
int64_t sys::detail::Syscall::fHandleIds(std::u8string_view name) const {
	logger.debug(u8"Syscall ", name, u8"()");
	return uint16_t(env::Instance()->getId());
}
int64_t sys::detail::Syscall::fHandleUName(env::guest_t addr) const {
	logger.info(u8"Assumption: Entries are [sysname, nodename, release, version, machine] with each 65 chars");

	/* write the systemname out */
	env::Instance()->memory().mwrite(addr + 0 * 65llu, u8"wasmlator", 10, env::Usage::Write);

	/* write the nodename out */
	env::Instance()->memory().mwrite(addr + 1 * 65llu, u8"", 1, env::Usage::Write);

	/* write the release out (version needs to be high enough for glibc to accept it) */
	env::Instance()->memory().mwrite(addr + 2 * 65llu, u8"6.8.0-wasmlator", 16, env::Usage::Write);

	/* write the version out */
	env::Instance()->memory().mwrite(addr + 3 * 65llu, u8"userspace-wasmlator #version 1.0.0", 35, env::Usage::Write);

	/* write the machine out */
	env::Instance()->memory().mwrite(addr + 4 * 65llu, u8"wasm", 5, env::Usage::Write);
	return errCode::eSuccess;
}

bool sys::detail::Syscall::setup(sys::Userspace* userspace, env::guest_t endOfData, std::u8string_view path) {
	pUserspace = userspace;
	pConfig.path = path;

	/* setup the file-io */
	if (!pFileIO.setup(this))
		return false;

	/* setup the memory interactions */
	if (!pMemory.setup(endOfData))
		return false;
	return true;
}
void sys::detail::Syscall::handle(env::guest_t address, env::guest_t nextAddress) {
	/* setup the new syscall */
	pCurrent.address = address;
	pCurrent.next = nextAddress;
	pCurrent.inplace = true;

	/* dispatch the call (will write the result back properly and continue execution at the right address) */
	fWrap([this]() { return fDispatch(); });
}

sys::detail::ProcessConfig& sys::detail::Syscall::config() {
	return pConfig;
}
sys::detail::FileIO& sys::detail::Syscall::fileIO() {
	return pFileIO;
}
void sys::detail::Syscall::callIncomplete() {
	pCurrent.inplace = false;
	throw detail::AwaitingSyscall{ pCurrent.next };
}
void sys::detail::Syscall::callContinue(std::function<int64_t()> callback) {
	fWrap(callback);
}
