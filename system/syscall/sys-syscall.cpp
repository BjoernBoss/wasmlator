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
			throw detail::AwaitingSyscall{};
		return;
	}

	/* write the result back */
	logger.debug(u8"result: ", str::As{ U"#018x", pCurrent.result });
	pUserspace->cpu()->syscallSetResult(pCurrent.result);

	/* check if this is not in-place execution, in which case the execution needs to be continued in this call */
	if (!inplace) {
		pUserspace->execute();
		return;
	}

	/* perform the check if the execution can continue at the next address (will also check for memory-invalidations) */
	pUserspace->checkContinue();
}
int64_t sys::detail::Syscall::fDispatch() {
	sys::SyscallArgs args = pUserspace->cpu()->syscallGetArgs();

	/* check if the syscall can be handled in-place */
	switch (args.index) {
	case sys::SyscallIndex::getuid: {
		logger.debug(u8"Syscall getuid()");
		return pMisc.getuid();
	}
	case sys::SyscallIndex::geteuid: {
		logger.debug(u8"Syscall geteuid()");
		return pMisc.geteuid();
	}
	case sys::SyscallIndex::getgid: {
		logger.debug(u8"Syscall getgid()");
		return pMisc.getgid();
	}
	case sys::SyscallIndex::getegid: {
		logger.debug(u8"Syscall getegid()");
		return pMisc.getegid();
	}
	case sys::SyscallIndex::gettimeofday: {
		logger.debug(u8"Syscall gettimeofday(", str::As{ U"#018x", args.args[0] }, u8", ", str::As{ U"#018x", args.args[1] }, u8')');
		return pMisc.gettimeofday(args.args[0], args.args[1]);
	}
	case sys::SyscallIndex::clock_gettime: {
		logger.debug(u8"Syscall clock_gettime(", args.args[0], u8", ", str::As{ U"#018x", args.args[1] }, u8')');
		return pMisc.clock_gettime(args.args[0], args.args[1]);
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
		logger.debug(u8"Syscall mmap(", str::As{ U"#018x", args.args[0] }, u8", ", str::As{ U"#010x", args.args[1] }, u8", ", args.args[2], u8", ", args.args[3], u8", ", int64_t(args.args[4]), u8", ", args.args[5], u8')');
		return pMemory.mmap(args.args[0], args.args[1], uint32_t(args.args[2]), uint32_t(args.args[3]), args.args[4], args.args[5]);
	}
	case sys::SyscallIndex::mprotect: {
		logger.debug(u8"Syscall mprotect(", str::As{ U"#018x", args.args[0] }, u8", ", str::As{ U"#010x", args.args[1] }, u8", ", args.args[2], u8')');
		return pMemory.mprotect(args.args[0], args.args[1], uint32_t(args.args[2]));
	}
	case sys::SyscallIndex::munmap: {
		logger.debug(u8"Syscall munmap(", str::As{ U"#018x", args.args[0] }, u8", ", str::As{ U"#010x", args.args[1] }, u8')');
		return pMemory.munmap(args.args[0], args.args[1]);
	}
	case sys::SyscallIndex::uname: {
		logger.debug(u8"Syscall uname(", str::As{ U"#018x", args.args[0] }, u8')');
		return pMisc.uname(args.args[0]);
	}
	case sys::SyscallIndex::sysinfo: {
		logger.debug(u8"Syscall sysinfo(", str::As{ U"#018x", args.args[0] }, u8')');
		return pMisc.sysinfo(args.args[0]);
	}
	case sys::SyscallIndex::getcwd: {
		logger.debug(u8"Syscall getcwd(", str::As{ U"#018x", args.args[0] }, u8", ", args.args[1], u8')');
		return pMisc.getcwd(args.args[0], args.args[1]);
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
	case sys::SyscallIndex::close: {
		logger.debug(u8"Syscall close(", int64_t(args.args[0]), u8')');
		return pFileIO.close(args.args[0]);
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
	case sys::SyscallIndex::fcntl: {
		logger.debug(u8"Syscall fcntl(", int64_t(args.args[0]), u8", ", args.args[1], u8", ", args.args[2], u8')');
		return pFileIO.fcntl(args.args[0], args.args[1], args.args[2]);
	}
	case sys::SyscallIndex::lseek: {
		logger.debug(u8"Syscall lseek(", int64_t(args.args[0]), u8", ", int64_t(args.args[1]), u8", ", args.args[2], u8')');
		return pFileIO.lseek(args.args[0], args.args[1], args.args[2]);
	}
	case sys::SyscallIndex::getdents: {
		logger.debug(u8"Syscall getdents(", int64_t(args.args[0]), u8", ", str::As{ U"#018x", args.args[1] }, u8", ", args.args[2], u8')');
		return pFileIO.getdents(args.args[0], args.args[1], args.args[2]);
	}
	case sys::SyscallIndex::set_tid_address: {
		logger.debug(u8"Syscall set_tid_address(", str::As{ U"#018x", args.args[0] }, u8')');
		return pMisc.set_tid_address(args.args[0]);
	}
	case sys::SyscallIndex::set_robust_list: {
		logger.debug(u8"Syscall set_robust_list(", str::As{ U"#018x", args.args[0] }, u8", ", args.args[1], u8')');
		return pMisc.set_robust_list(args.args[0], args.args[1]);
	}
	case sys::SyscallIndex::prlimit64: {
		logger.debug(u8"Syscall prlimit64(", args.args[0], u8", ", args.args[1], u8", ", str::As{ U"#018x", args.args[2] }, u8", ", str::As{ U"#018x", args.args[3] }, u8')');
		return pMisc.prlimit64(args.args[0], args.args[1], args.args[2], args.args[3]);
	}
	case sys::SyscallIndex::getrandom: {
		logger.debug(u8"Syscall getrandom(", str::As{ U"#018x", args.args[0] }, u8", ", args.args[1], u8", ", args.args[2], u8')');
		return pMisc.getrandom(args.args[0], args.args[1], uint32_t(args.args[2]));
	}
	case sys::SyscallIndex::completed: {
		logger.debug(u8"Syscall completed(index: ", args.rawIndex, u8')');
		return args.args[0];
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

bool sys::detail::Syscall::setup(sys::Userspace* userspace, env::guest_t endOfData, std::u8string_view path, std::u8string_view machine) {
	pUserspace = userspace;
	pConfig.path = std::u8string{ path };
	pConfig.username = u8"some-unknown";
	pConfig.machine = std::u8string{ machine };

	/* setup the file-io */
	if (!pFileIO.setup(this))
		return false;

	/* setup the memory interactions */
	if (!pMemory.setup(this, endOfData))
		return false;

	/* setup the remaining syscalls */
	if (!pMisc.setup(this))
		return false;
	return true;
}
void sys::detail::Syscall::handle(env::guest_t address, env::guest_t nextAddress) {
	/* update the pc to already point to the next address */
	pUserspace->setPC(nextAddress);

	/* setup the new syscall */
	pCurrent.address = address;
	pCurrent.completed = false;
	pCurrent.nested = 0;

	/* dispatch the call (will write the result back properly and continue execution at the right address) */
	fWrap(true, [this]() { return fDispatch(); });
}

sys::detail::ProcessConfig& sys::detail::Syscall::process() {
	return pConfig;
}
sys::detail::FileIO& sys::detail::Syscall::files() {
	return pFileIO;
}
int64_t sys::detail::Syscall::callIncomplete() {
	throw detail::AwaitingSyscall{};
}
void sys::detail::Syscall::callContinue(std::function<int64_t()> callback) {
	fWrap(false, callback);
}
