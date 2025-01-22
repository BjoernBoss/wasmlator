#include "../system.h"

static util::Logger logger{ u8"sys::syscall" };

std::u8string sys::detail::Syscall::fReadString(env::guest_t address) const {
	env::Memory& mem = env::Instance()->memory();
	std::u8string out;

	/* read the string from the guest memory */
	for (char8_t c = mem.read<char8_t>(address++); c != 0; c = mem.read<char8_t>(address++))
		out.push_back(c);

	return out;
}

void sys::detail::Syscall::fWrap(bool inplace, std::function<int64_t()> callback) {
	/* execute the wrapped call (to catch any memory exceptions and nested incomplete-calls) */
	++pCurrent.nested;
	try {
		pCurrent.result = callback();
		pCurrent.completed = true;
	}
	catch (const env::MemoryFault& e) {
		logger.debug(u8"Memory fault at [", str::As{ U"#018x", e.accessed }, u8"] while handling syscall");
		pCurrent.result = errCode::eFault;
		pCurrent.completed = true;
	}
	catch (const detail::AwaitingSyscall&) {}
	--pCurrent.nested;

	/* check if this is a nested call, or the call has not yet been completed, in which
	*	case the execution needs to be propagated further down (exception cannot be
	*	thrown from outside, as they will otherwise be passed out of the application) */
	if (pCurrent.nested > 0 || !pCurrent.completed) {
		if (inplace)
			throw detail::AwaitingSyscall{ pCurrent.next };
		return;
	}

	/* write the result back */
	logger.debug(u8"result: ", str::As{ U"#018x", pCurrent.result });
	pUserspace->cpu()->syscallSetResult(pCurrent.result);

	/* check if this is not in-place execution, in which case the execution needs to be restarted at the
	*	next address (pc will already have been set by at least one await-syscall exception being thrown) */
	if (!inplace)
		pUserspace->execute();
}
int64_t sys::detail::Syscall::fDispatch() {
	sys::SyscallArgs args = pUserspace->cpu()->syscallGetArgs();

	/* check if the syscall can be handled in-place */
	switch (args.index) {
	case sys::SyscallIndex::getuid: {
		logger.debug(u8"Syscall getuid()");
		return pConfig.uid;
	}
	case sys::SyscallIndex::geteuid: {
		logger.debug(u8"Syscall geteuid()");
		return pConfig.euid;
	}
	case sys::SyscallIndex::getgid: {
		logger.debug(u8"Syscall getgid()");
		return pConfig.gid;
	}
	case sys::SyscallIndex::getegid: {
		logger.debug(u8"Syscall getegid()");
		return pConfig.egid;
	}
	case sys::SyscallIndex::exit_group: {
		logger.debug(u8"Syscall exit_group(", args.args[0], u8')');
		env::Instance()->context().terminate(int32_t(int64_t(args.args[0])), pCurrent.address);
		return errCode::eSuccess;
	}
	case sys::SyscallIndex::brk: {
		logger.debug(u8"Syscall brk(", str::As{ U"#018x", args.args[0] }, u8')');
		return pMemory.brk(args.args[0]);
	}
	case sys::SyscallIndex::mmap: {
		logger.debug(u8"Syscall mmap(", str::As{ U"#018x", args.args[0] }, u8", ", str::As{ U"#010x", args.args[1] }, u8", ", args.args[2], u8", ", args.args[3], u8", ", args.args[4], u8", ", args.args[5], u8')');
		return pMemory.mmap(args.args[0], args.args[1], uint32_t(args.args[2]), uint32_t(args.args[3]), args.args[4], args.args[5]);
	}
	case sys::SyscallIndex::uname: {
		logger.debug(u8"Syscall uname(", str::As{ U"#018x", args.args[0] }, u8')');
		return fHandleUName(args.args[0]);
	}
	case sys::SyscallIndex::openat: {
		logger.debug(u8"Syscall openat(", int64_t(args.args[0]), u8", ", str::As{ U"#018x", args.args[1] }, u8", ", args.args[2], u8", ", args.args[3], u8')');
		std::u8string path = fReadString(args.args[1]);
		logger.debug(u8"pathname: [", path, u8']');
		return pFileIO.openat(args.args[0], path, args.args[2], args.args[3]);
	}
	case sys::SyscallIndex::open: {
		logger.debug(u8"Syscall open(", str::As{ U"#018x", args.args[0] }, u8", ", args.args[1], u8", ", args.args[2], u8')');
		std::u8string path = fReadString(args.args[0]);
		logger.debug(u8"pathname: [", path, u8']');
		return pFileIO.open(path, args.args[1], args.args[2]);
	}
	case sys::SyscallIndex::readv: {
		logger.debug(u8"Syscall readv(", int64_t(args.args[0]), u8", ", str::As{ U"#018x", args.args[1] }, u8", ", args.args[2], u8')');
		return pFileIO.readv(args.args[0], args.args[1], args.args[2]);
	}
	case sys::SyscallIndex::read: {
		logger.debug(u8"Syscall read(", int64_t(args.args[0]), u8", ", str::As{ U"#018x", args.args[1] }, u8", ", args.args[2], u8')');
		return pFileIO.read(args.args[0], args.args[1], args.args[2]);
	}
	case sys::SyscallIndex::writev: {
		logger.debug(u8"Syscall writev(", int64_t(args.args[0]), u8", ", str::As{ U"#018x", args.args[1] }, u8", ", args.args[2], u8')');
		return pFileIO.writev(args.args[0], args.args[1], args.args[2]);
	}
	case sys::SyscallIndex::write: {
		logger.debug(u8"Syscall write(", int64_t(args.args[0]), u8", ", str::As{ U"#018x", args.args[1] }, u8", ", args.args[2], u8')');
		return pFileIO.write(args.args[0], args.args[1], args.args[2]);
	}
	case sys::SyscallIndex::readlinkat: {
		logger.debug(u8"Syscall readlinkat(", int64_t(args.args[0]), u8", ", str::As{ U"#018x", args.args[1] }, u8", ", args.args[2], u8", ", args.args[3], u8')');
		std::u8string path = fReadString(args.args[1]);
		logger.debug(u8"pathname: [", path, u8']');
		return pFileIO.readlinkat(args.args[0], path, args.args[2], args.args[3]);
	}
	case sys::SyscallIndex::readlink: {
		logger.debug(u8"Syscall readlink(", str::As{ U"#018x", args.args[0] }, u8", ", args.args[1], u8", ", args.args[2], u8')');
		std::u8string path = fReadString(args.args[0]);
		logger.debug(u8"pathname: [", path, u8']');
		return pFileIO.readlink(path, args.args[1], args.args[2]);
	}
	case sys::SyscallIndex::fstat: {
		logger.debug(u8"Syscall fstat(", int64_t(args.args[0]), u8", ", str::As{ U"#018x", args.args[1] }, u8')');
		return pFileIO.fstat(int64_t(args.args[0]), args.args[1]);
	}
	case sys::SyscallIndex::fstatat: {
		logger.debug(u8"Syscall fstatat(", int64_t(args.args[0]), u8", ", str::As{ U"#018x", args.args[1] }, u8", ", str::As{ U"#018x", args.args[2] }, u8", ", args.args[3], u8')');
		std::u8string path = fReadString(args.args[1]);
		logger.debug(u8"pathname: [", path, u8']');
		return pFileIO.fstatat(args.args[0], path, args.args[2], args.args[3]);
	}
	case sys::SyscallIndex::access: {
		logger.debug(u8"Syscall access(", str::As{ U"#018x", args.args[0] }, u8", ", args.args[1], u8')');
		std::u8string path = fReadString(args.args[0]);
		logger.debug(u8"pathname: [", path, u8']');
		return pFileIO.access(path, args.args[1]);
	}
	case sys::SyscallIndex::faccessat: {
		logger.debug(u8"Syscall faccessat(", int64_t(args.args[0]), u8", ", str::As{ U"#018x", args.args[1] }, u8", ", args.args[2], u8')');
		std::u8string path = fReadString(args.args[1]);
		logger.debug(u8"pathname: [", path, u8']');
		return pFileIO.faccessat(args.args[0], path, args.args[2]);
	}
	case sys::SyscallIndex::faccessat2: {
		logger.debug(u8"Syscall faccessat2(", int64_t(args.args[0]), u8", ", str::As{ U"#018x", args.args[1] }, u8", ", args.args[2], u8", ", args.args[3], u8')');
		std::u8string path = fReadString(args.args[1]);
		logger.debug(u8"pathname: [", path, u8']');
		return pFileIO.faccessat2(args.args[0], path, args.args[2], args.args[3]);
	}
	case sys::SyscallIndex::ioctl: {
		logger.debug(u8"Syscall ioctl(", int64_t(args.args[0]), u8", ", args.args[1], u8", ", args.args[2], u8')');
		return pFileIO.ioctl(args.args[0], args.args[1], args.args[2]);
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
int64_t sys::detail::Syscall::fHandleUName(env::guest_t addr) const {
	/* helper function */
	auto write = [](char8_t(&buffer)[65], std::u8string_view str) {
		std::memcpy(buffer, str.data(), std::min<size_t>(sizeof(buffer), str.size() + 1));
		};

	/* construct the uname-content */
	linux::UName uname;
	write(uname.sysName, u8"wasmlator");
	write(uname.nodeName, pConfig.userName);
	write(uname.release, u8"6.8.0-wasmlator");
	write(uname.version, u8"userspace-wasmlator #version 1.0.0");
	write(uname.machine, pMachine);

	/* write the data to the guest */
	env::Instance()->memory().mwrite(addr, &uname, sizeof(linux::UName), env::Usage::Write);
	return errCode::eSuccess;
}

bool sys::detail::Syscall::setup(sys::Userspace* userspace, env::guest_t endOfData, std::u8string_view path, std::u8string_view machine) {
	pUserspace = userspace;
	pMachine = std::u8string{ machine };
	pConfig.path = std::u8string{ path };
	pConfig.userName = u8"some-unknown";

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
	pCurrent.completed = false;
	pCurrent.nested = 0;

	/* dispatch the call (will write the result back properly and continue execution at the right address) */
	fWrap(true, [this]() { return fDispatch(); });
}

sys::detail::ProcessConfig& sys::detail::Syscall::config() {
	return pConfig;
}
sys::detail::FileIO& sys::detail::Syscall::fileIO() {
	return pFileIO;
}
int64_t sys::detail::Syscall::callIncomplete() {
	throw detail::AwaitingSyscall{ pCurrent.next };
}
void sys::detail::Syscall::callContinue(std::function<int64_t()> callback) {
	fWrap(false, callback);
}
