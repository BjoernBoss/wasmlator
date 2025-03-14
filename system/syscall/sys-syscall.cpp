/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025 Bjoern Boss Henrichsen */
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
	sys::SyscallArgs syscall = pUserspace->cpu()->syscallGetArgs();
	const uint64_t(&args)[6] = syscall.args;

	/* check if the syscall can be handled in-place */
	switch (syscall.index) {
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
	case sys::SyscallIndex::getpid: {
		logger.debug(u8"Syscall getpid()");
		return pMisc.getpid();
	}
	case sys::SyscallIndex::gettid: {
		logger.debug(u8"Syscall gettid()");
		return pMisc.gettid();
	}
	case sys::SyscallIndex::gettimeofday: {
		logger.debug(u8"Syscall gettimeofday(", str::As{ U"#018x", args[0] }, u8", ", str::As{ U"#018x", args[1] }, u8')');
		return pMisc.gettimeofday(args[0], args[1]);
	}
	case sys::SyscallIndex::clock_gettime: {
		logger.debug(u8"Syscall clock_gettime(", args[0], u8", ", str::As{ U"#018x", args[1] }, u8')');
		return pMisc.clock_gettime(args[0], args[1]);
	}
	case sys::SyscallIndex::exit_group: {
		logger.debug(u8"Syscall exit_group(", args[0], u8')');
		env::Instance()->context().terminate(int32_t(int64_t(args[0])), pCurrent.address);
		return errCode::eSuccess;
	}
	case sys::SyscallIndex::tgkill: {
		logger.debug(u8"Syscall tgkill(", args[0], args[1], int64_t(args[2]), u8')');
		return pMisc.tgkill(args[0], args[1], int64_t(args[2]));
	}
	case sys::SyscallIndex::brk: {
		logger.debug(u8"Syscall brk(", str::As{ U"#018x", args[0] }, u8')');
		return pMemory.brk(args[0]);
	}
	case sys::SyscallIndex::mmap: {
		logger.debug(u8"Syscall mmap(", str::As{ U"#018x", args[0] }, u8", ", str::As{ U"#010x", args[1] }, u8", ", args[2], u8", ", args[3], u8", ", int64_t(args[4]), u8", ", args[5], u8')');
		return pMemory.mmap(args[0], args[1], uint32_t(args[2]), uint32_t(args[3]), args[4], args[5]);
	}
	case sys::SyscallIndex::mprotect: {
		logger.debug(u8"Syscall mprotect(", str::As{ U"#018x", args[0] }, u8", ", str::As{ U"#010x", args[1] }, u8", ", args[2], u8')');
		return pMemory.mprotect(args[0], args[1], uint32_t(args[2]));
	}
	case sys::SyscallIndex::munmap: {
		logger.debug(u8"Syscall munmap(", str::As{ U"#018x", args[0] }, u8", ", str::As{ U"#010x", args[1] }, u8')');
		return pMemory.munmap(args[0], args[1]);
	}
	case sys::SyscallIndex::mremap: {
		logger.debug(u8"Syscall mremap(", str::As{ U"#018x", args[0] }, u8", ", str::As{ U"#010x", args[1] }, u8", ", str::As{ U"#010x", args[2] }, u8", ", args[3], u8", ", str::As{ U"#018x", args[4] }, u8')');
		return pMemory.mremap(args[0], args[1], args[2], uint32_t(args[3]), args[4]);
	}
	case sys::SyscallIndex::uname: {
		logger.debug(u8"Syscall uname(", str::As{ U"#018x", args[0] }, u8')');
		return pMisc.uname(args[0]);
	}
	case sys::SyscallIndex::sysinfo: {
		logger.debug(u8"Syscall sysinfo(", str::As{ U"#018x", args[0] }, u8')');
		return pMisc.sysinfo(args[0]);
	}
	case sys::SyscallIndex::getcwd: {
		logger.debug(u8"Syscall getcwd(", str::As{ U"#018x", args[0] }, u8", ", args[1], u8')');
		return pMisc.getcwd(args[0], args[1]);
	}
	case sys::SyscallIndex::chdir: {
		logger.debug(u8"Syscall chdir(", str::As{ U"#018x", args[0] }, u8')');
		std::u8string path = fReadString(args[0]);
		logger.debug(u8"pathname: [", path, u8']');
		return pFileIO.chdir(path);
	}
	case sys::SyscallIndex::openat: {
		logger.debug(u8"Syscall openat(", int64_t(args[0]), u8", ", str::As{ U"#018x", args[1] }, u8", ", args[2], u8", ", args[3], u8')');
		std::u8string path = fReadString(args[1]);
		logger.debug(u8"pathname: [", path, u8']');
		return pFileIO.openat(args[0], path, args[2], args[3]);
	}
	case sys::SyscallIndex::open: {
		logger.debug(u8"Syscall open(", str::As{ U"#018x", args[0] }, u8", ", args[1], u8", ", args[2], u8')');
		std::u8string path = fReadString(args[0]);
		logger.debug(u8"pathname: [", path, u8']');
		return pFileIO.open(path, args[1], args[2]);
	}
	case sys::SyscallIndex::close: {
		logger.debug(u8"Syscall close(", int64_t(args[0]), u8')');
		return pFileIO.close(args[0]);
	}
	case sys::SyscallIndex::readv: {
		logger.debug(u8"Syscall readv(", int64_t(args[0]), u8", ", str::As{ U"#018x", args[1] }, u8", ", args[2], u8')');
		return pFileIO.readv(args[0], args[1], args[2]);
	}
	case sys::SyscallIndex::read: {
		logger.debug(u8"Syscall read(", int64_t(args[0]), u8", ", str::As{ U"#018x", args[1] }, u8", ", args[2], u8')');
		return pFileIO.read(args[0], args[1], args[2]);
	}
	case sys::SyscallIndex::writev: {
		logger.debug(u8"Syscall writev(", int64_t(args[0]), u8", ", str::As{ U"#018x", args[1] }, u8", ", args[2], u8')');
		return pFileIO.writev(args[0], args[1], args[2]);
	}
	case sys::SyscallIndex::write: {
		logger.debug(u8"Syscall write(", int64_t(args[0]), u8", ", str::As{ U"#018x", args[1] }, u8", ", args[2], u8')');
		return pFileIO.write(args[0], args[1], args[2]);
	}
	case sys::SyscallIndex::readlinkat: {
		logger.debug(u8"Syscall readlinkat(", int64_t(args[0]), u8", ", str::As{ U"#018x", args[1] }, u8", ", args[2], u8", ", args[3], u8')');
		std::u8string path = fReadString(args[1]);
		logger.debug(u8"pathname: [", path, u8']');
		return pFileIO.readlinkat(args[0], path, args[2], args[3]);
	}
	case sys::SyscallIndex::readlink: {
		logger.debug(u8"Syscall readlink(", str::As{ U"#018x", args[0] }, u8", ", args[1], u8", ", args[2], u8')');
		std::u8string path = fReadString(args[0]);
		logger.debug(u8"pathname: [", path, u8']');
		return pFileIO.readlink(path, args[1], args[2]);
	}
	case sys::SyscallIndex::fstat: {
		logger.debug(u8"Syscall fstat(", int64_t(args[0]), u8", ", str::As{ U"#018x", args[1] }, u8')');
		return pFileIO.fstat(int64_t(args[0]), args[1]);
	}
	case sys::SyscallIndex::fstatat: {
		logger.debug(u8"Syscall fstatat(", int64_t(args[0]), u8", ", str::As{ U"#018x", args[1] }, u8", ", str::As{ U"#018x", args[2] }, u8", ", args[3], u8')');
		std::u8string path = fReadString(args[1]);
		logger.debug(u8"pathname: [", path, u8']');
		return pFileIO.fstatat(args[0], path, args[2], args[3]);
	}
	case sys::SyscallIndex::access: {
		logger.debug(u8"Syscall access(", str::As{ U"#018x", args[0] }, u8", ", args[1], u8')');
		std::u8string path = fReadString(args[0]);
		logger.debug(u8"pathname: [", path, u8']');
		return pFileIO.access(path, args[1]);
	}
	case sys::SyscallIndex::faccessat: {
		logger.debug(u8"Syscall faccessat(", int64_t(args[0]), u8", ", str::As{ U"#018x", args[1] }, u8", ", args[2], u8')');
		std::u8string path = fReadString(args[1]);
		logger.debug(u8"pathname: [", path, u8']');
		return pFileIO.faccessat(args[0], path, args[2]);
	}
	case sys::SyscallIndex::faccessat2: {
		logger.debug(u8"Syscall faccessat2(", int64_t(args[0]), u8", ", str::As{ U"#018x", args[1] }, u8", ", args[2], u8", ", args[3], u8')');
		std::u8string path = fReadString(args[1]);
		logger.debug(u8"pathname: [", path, u8']');
		return pFileIO.faccessat2(args[0], path, args[2], args[3]);
	}
	case sys::SyscallIndex::ioctl: {
		logger.debug(u8"Syscall ioctl(", int64_t(args[0]), u8", ", args[1], u8", ", args[2], u8')');
		return pFileIO.ioctl(args[0], args[1], args[2]);
	}
	case sys::SyscallIndex::fcntl: {
		logger.debug(u8"Syscall fcntl(", int64_t(args[0]), u8", ", args[1], u8", ", args[2], u8')');
		return pFileIO.fcntl(args[0], args[1], args[2]);
	}
	case sys::SyscallIndex::lseek: {
		logger.debug(u8"Syscall lseek(", int64_t(args[0]), u8", ", int64_t(args[1]), u8", ", args[2], u8')');
		return pFileIO.lseek(args[0], args[1], args[2]);
	}
	case sys::SyscallIndex::getdents64: {
		logger.debug(u8"Syscall getdents64(", int64_t(args[0]), u8", ", str::As{ U"#018x", args[1] }, u8", ", args[2], u8')');
		return pFileIO.getdents64(args[0], args[1], args[2]);
	}
	case sys::SyscallIndex::set_tid_address: {
		logger.debug(u8"Syscall set_tid_address(", str::As{ U"#018x", args[0] }, u8')');
		return pMisc.set_tid_address(args[0]);
	}
	case sys::SyscallIndex::set_robust_list: {
		logger.debug(u8"Syscall set_robust_list(", str::As{ U"#018x", args[0] }, u8", ", args[1], u8')');
		return pMisc.set_robust_list(args[0], args[1]);
	}
	case sys::SyscallIndex::prlimit64: {
		logger.debug(u8"Syscall prlimit64(", args[0], u8", ", args[1], u8", ", str::As{ U"#018x", args[2] }, u8", ", str::As{ U"#018x", args[3] }, u8')');
		return pMisc.prlimit64(args[0], args[1], args[2], args[3]);
	}
	case sys::SyscallIndex::getrandom: {
		logger.debug(u8"Syscall getrandom(", str::As{ U"#018x", args[0] }, u8", ", args[1], u8", ", args[2], u8')');
		return pMisc.getrandom(args[0], args[1], uint32_t(args[2]));
	}
	case sys::SyscallIndex::futex: {
		logger.debug(u8"Syscall futex(", str::As{ U"#018x", args[0] }, u8", ", int64_t(args[1]), u8", ", uint32_t(args[2]), u8", ", str::As{ U"#010x", args[3] }, u8", ", str::As{ U"#010x", args[4] }, u8", ", uint32_t(args[5]), u8')');
		return pMisc.futex(args[0], int64_t(args[1]), uint32_t(args[2]), args[3], args[4], uint32_t(args[5]));
	}
	case sys::SyscallIndex::rt_sigprocmask: {
		logger.debug(u8"Syscall rt_sigprocmask(", int64_t(args[0]), u8", ", str::As{ U"#018x", args[1] }, u8", ", str::As{ U"#010x", args[2] }, u8", ", args[3], u8')');
		return pMisc.rt_sigprocmask(int64_t(args[0]), args[1], args[2], args[3]);
	}
	case sys::SyscallIndex::completed: {
		logger.debug(u8"Syscall completed(index: ", syscall.rawIndex, u8')');
		return args[0];
	}
	case sys::SyscallIndex::unknown:
		throw detail::UnknownSyscall{ pCurrent.address, syscall.rawIndex };
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

const sys::detail::ProcessConfig& sys::detail::Syscall::process() const {
	return pConfig;
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
