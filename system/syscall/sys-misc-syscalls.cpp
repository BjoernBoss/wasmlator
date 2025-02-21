#include "../system.h"

static util::Logger logger{ u8"sys::syscall" };

bool sys::detail::MiscSyscalls::setup(detail::Syscall* syscall) {
	pSyscall = syscall;
	return true;
}
int64_t sys::detail::MiscSyscalls::getuid() const {
	return pSyscall->process().uid;
}
int64_t sys::detail::MiscSyscalls::geteuid() const {
	return pSyscall->process().euid;
}
int64_t sys::detail::MiscSyscalls::getgid() const {
	return pSyscall->process().gid;
}
int64_t sys::detail::MiscSyscalls::getegid() const {
	return pSyscall->process().egid;
}
int64_t sys::detail::MiscSyscalls::getpid() const {
	return pSyscall->process().pid;
}
int64_t sys::detail::MiscSyscalls::gettid() const {
	return pSyscall->process().tid;
}
int64_t sys::detail::MiscSyscalls::uname(env::guest_t address) const {
	/* helper function */
	auto write = [](char8_t(&buffer)[65], std::u8string_view str) {
		std::memcpy(buffer, str.data(), std::min<size_t>(sizeof(buffer), str.size() + 1));
		};

	/* construct the uname-content */
	linux::UName uname;
	write(uname.sysName, u8"wasmlator");
	write(uname.nodeName, pSyscall->process().username);
	write(uname.release, u8"6.8.0-wasmlator");
	write(uname.version, u8"userspace-wasmlator #version 1.0.0");
	write(uname.machine, pSyscall->process().machine);

	/* write the data to the guest */
	env::Instance()->memory().mwrite(address, &uname, sizeof(linux::UName), env::Usage::Write);
	return errCode::eSuccess;
}
int64_t sys::detail::MiscSyscalls::sysinfo(env::guest_t info) const {
	/* populate the sys-info structure */
	linux::SysInfo out{ 0 };

	/* populate the uptime */
	out.uptime = (host::GetStampUS() - env::Instance()->startTimeUS()) / 1000'000;

	/* setup the system to be fully loaded with this single process */
	out.loads[0] = uint64_t(consts::fullSingleCoreLoad * std::min<double>(1.0, out.uptime / (1 * 60.0)));
	out.loads[1] = uint64_t(consts::fullSingleCoreLoad * std::min<double>(1.0, out.uptime / (5 * 60.0)));
	out.loads[2] = uint64_t(consts::fullSingleCoreLoad * std::min<double>(1.0, out.uptime / (15 * 60.0)));

	/* setup the ram parameter */
	out.totalram = env::Instance()->memory().maxAllocate();
	out.freeram = out.totalram - env::Instance()->memory().totalAllocated();
	out.sharedram = env::Instance()->memory().totalShared();
	out.bufferram = 0;
	out.totalswap = 0;
	out.freeswap = 0;
	out.processes = 1;
	out.totalhigh = 0;
	out.freehigh = 0;
	out.mem_unit = 1;

	/* write the structure back out */
	env::Instance()->memory().mwrite(info, &out, sizeof(linux::SysInfo), env::Usage::Write);
	return errCode::eSuccess;
}
int64_t sys::detail::MiscSyscalls::getcwd(env::guest_t buf, uint64_t size) const {
	const std::u8string& path = pSyscall->process().workingDirectory;

	/* validate the paramter */
	if (size != 0 && buf == 0)
		return errCode::eInvalid;
	if (size < path.size() + 1)
		return errCode::eRange;

	/* write the path to memory */
	env::Instance()->memory().mwrite(buf, path.data(), path.size() + 1, env::Usage::Write);
	return errCode::eSuccess;
}
int64_t sys::detail::MiscSyscalls::gettimeofday(env::guest_t tv, env::guest_t tz) const {
	/* check if the timevalue should be written out */
	if (tv != 0) {
		uint64_t time = host::GetStampUS();
		linux::TimeValue out;
		out.sec = (time / 1000'000);
		out.usec = (time % 1000'000);
		env::Instance()->memory().mwrite(tv, &out, sizeof(linux::TimeValue), env::Usage::Write);
	}

	/* check if the timezone should be written out */
	if (tz != 0) {
		int32_t zone = host::GetTimeZoneMIN();
		linux::TimeZone out;
		out.minuteswest = -zone;
		out.dsttime = 0;
		env::Instance()->memory().mwrite(tz, &out, sizeof(linux::TimeZone), env::Usage::Write);
	}
	return errCode::eSuccess;
}
int64_t sys::detail::MiscSyscalls::clock_gettime(uint64_t clockid, env::guest_t tp) const {
	/* validate the clockid */
	if (clockid != consts::clockRealTime && clockid != consts::clockMonotonic)
		return errCode::eInvalid;

	/* fetch the time and setup the output structure */
	linux::TimeSpec time = { 0 };
	uint64_t stamp = host::GetStampUS();
	time.sec = stamp / 1000'000;
	time.nsec = (stamp % 1000'000) * 1000;

	/* write the time back */
	env::Instance()->memory().mwrite(tp, &time, sizeof(linux::TimeSpec), env::Usage::Write);
	return errCode::eSuccess;
}
int64_t sys::detail::MiscSyscalls::set_tid_address(env::guest_t tidptr) const {
	pSyscall->process().clear_child_tid = tidptr;
	return pSyscall->process().tid;
}
int64_t sys::detail::MiscSyscalls::set_robust_list(env::guest_t head, uint64_t size) const {
	logger.warn(u8"Unsupported syscall set_robust_list executed");
	return errCode::eNotImplemented;
}
int64_t sys::detail::MiscSyscalls::prlimit64(uint64_t pid, uint64_t res, env::guest_t new_rlim, env::guest_t old_rlim) const {
	/* validate the limit-type */
	if (res >= consts::rLimitEnd)
		return errCode::eInvalid;

	/* check if a limit is trying to be written (not allowed for any process) */
	if (new_rlim != 0)
		return errCode::ePermissionDenied;

	/* check if a limit is being read */
	if (old_rlim == 0)
		return errCode::eSuccess;

	/* construct the requested limit */
	linux::ResourceLimit limit = { 0 };
	switch (res) {
	case consts::rLimitCpu:
		limit = { consts::rLimitInfinity, consts::rLimitInfinity };
		break;
	case consts::rLimitFSize:
		limit = { consts::rLimitInfinity, consts::rLimitInfinity };
		break;
	case consts::rLimitData:
		limit = { consts::rLimitInfinity, consts::rLimitInfinity };
		break;
	case consts::rLimitStack:
		limit = { detail::StackSize, detail::StackSize };
		break;
	case consts::rLimitCore:
		limit = { 0, 0 };
		break;
	case consts::rLimitRSS:
		limit = { consts::rLimitInfinity, consts::rLimitInfinity };
		break;
	case consts::rLimitNumProc:
		limit = { detail::MaxProcessCount, detail::MaxProcessCount };
		break;
	case consts::rLimitNumFile:
		limit = { detail::MaxFileDescriptors, detail::MaxFileDescriptors };
		break;
	case consts::rLimitMemLock:
		limit = { detail::MaxLockedMemory, detail::MaxLockedMemory };
		break;
	case consts::rLimitAddressSpace:
		limit = { consts::rLimitInfinity, consts::rLimitInfinity };
		break;
	}

	/* write the structure back */
	env::Instance()->memory().mwrite(old_rlim, &limit, sizeof(linux::ResourceLimit), env::Usage::Write);
	return errCode::eSuccess;
}
int64_t sys::detail::MiscSyscalls::getrandom(env::guest_t buf, uint64_t buflen, uint32_t flags) const {
	/* check if all flags are supported */
	if ((flags & ~consts::randomMask) != 0)
		return errCode::eInvalid;

	/* check if true random data are requested */
	if (detail::IsSet(flags, consts::randomRandom))
		logger.fatal(u8"true random data not supported by getrandom");

	/* fill the buffer with random data */
	for (size_t i = 0; i < buflen; i += 4) {
		uint32_t val = host::Random();
		env::Instance()->memory().mwrite(buf + i, &val, (i + 4 < buflen ? 4 : buflen - i), env::Usage::Write);
	}
	return buflen;
}
int64_t sys::detail::MiscSyscalls::futex(env::guest_t uaddr, int64_t futex_op, uint32_t val, env::guest_t timeout, env::guest_t uaddr2, uint32_t val3) const {
	logger.warn(u8"unsupported syscall futex used");
	return errCode::eNotImplemented;
}
int64_t sys::detail::MiscSyscalls::rt_sigprocmask(int64_t how, env::guest_t set, env::guest_t oldset, uint64_t sigsetsize) const {
	logger.warn(u8"unsupported syscall rt_sigprocmask used");
	return errCode::eNotImplemented;
}
int64_t sys::detail::MiscSyscalls::tgkill(uint64_t tgid, uint64_t tid, int64_t sig) const {
	logger.warn(u8"thread killing not supported");
	return errCode::eNotImplemented;
}
