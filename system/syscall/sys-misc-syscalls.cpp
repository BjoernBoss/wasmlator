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
int64_t sys::detail::MiscSyscalls::gettimeofday(env::guest_t tv, env::guest_t tz) const {
	/* check if the timevalue should be written out */
	if (tv != 0) {
		uint64_t time = host::GetStampUS();
		linux::TimeValue out;
		out.sec = (time / 1'000'000);
		out.usec = (time % 1'000'000);
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
