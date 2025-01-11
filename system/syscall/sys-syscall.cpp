#include "../system.h"

static host::Logger logger{ u8"sys::Syscall" };

std::u8string sys::detail::Syscall::fReadString(env::guest_t address) const {
	env::Memory& mem = env::Instance()->memory();
	std::u8string out;

	/* read the string from the guest memory */
	for (char8_t c = mem.read<char8_t>(address++); c != 0; c = mem.read<char8_t>(address++))
		out.push_back(c);

	return out;
}

void sys::detail::Syscall::fHandle(env::guest_t address) {
	sys::Cpu* cpu = pUserspace->cpu();

	/* catch any potential memory errors */
	uint64_t result = 0;
	try {
		result = fWrapped(address, cpu->syscallGetArgs());
	}
	catch (const env::MemoryFault& e) {
		logger.debug(u8"Memory fault at [", str::As{ U"#018x", e.accessed }, u8"] while handling syscall");
		result = errCode::eFault;
	}

	/* write the result out to the process */
	logger.debug(u8"result: ", str::As{ U"#018x", int64_t(result) });
	cpu->syscallSetResult(result);
}
uint64_t sys::detail::Syscall::fWrapped(env::guest_t address, const sys::SyscallArgs& args) {
	/* check if the syscall can be handled in-place */
	switch (args.index) {
	case sys::SyscallIndex::read:
		logger.fatal(u8"Syscall [read] not implemented");
		break;
	case sys::SyscallIndex::write:
		logger.fatal(u8"Syscall [write] not implemented");
		break;
	case sys::SyscallIndex::getuid:
		return fHandleIds(u8"getuid");
	case sys::SyscallIndex::geteuid:
		return fHandleIds(u8"geteuid");
	case sys::SyscallIndex::getgid:
		return fHandleIds(u8"getgid");
	case sys::SyscallIndex::getegid:
		return fHandleIds(u8"getegid");
	case sys::SyscallIndex::brk:
		return fHandleBrk(args.args[0]);
	case sys::SyscallIndex::uname:
		return fHandleUName(args.args[0]);
	case sys::SyscallIndex::openat:
		return fHandleOpenAt(args.args[0], args.args[1], args.args[2], args.args[3]);
	case sys::SyscallIndex::unknown:
		throw detail::UnknownSyscall{ address, args.rawIndex };
		break;
	default:
		logger.fatal(u8"Syscall currently not implemented");
		break;
	}

	/* should never be reached */
	return errCode::eUnknown;
}
uint64_t sys::detail::Syscall::fHandleIds(std::u8string_view name) const {
	logger.debug(u8"Syscall ", name, u8"()");
	return uint16_t(env::Instance()->getId());
}
env::guest_t sys::detail::Syscall::fHandleBrk(env::guest_t addr) {
	logger.debug(u8"Syscall brk(", str::As{ U"#018x", addr }, u8')');

	/* check if the address lies beneath the initial address, in which case
	*	the current break can just be returned, as no changes will be made */
	if (addr < pBrk.init)
		return pBrk.current;

	/* page-align the actual address to determine if allocations need to be made or not */
	env::guest_t aligned = ((addr + pPageSize - 1) & ~(pPageSize - 1));

	/* check if memory can be released (ignore failure of unmapping the memory) */
	if (aligned < pBrk.aligned) {
		if (!env::Instance()->memory().munmap(aligned, uint32_t(pBrk.aligned - aligned)))
			logger.warn(u8"Unable to release break-memory");
		else
			pBrk.aligned = aligned;
	}

	/* check if memory needs to be allocated */
	else if (aligned > pBrk.aligned) {
		if (!env::Instance()->memory().mmap(pBrk.aligned, uint32_t(aligned - pBrk.aligned), env::Usage::ReadWrite)) {
			logger.warn(u8"Unable to allocate break-memory");
			return pBrk.current;
		}
		pBrk.aligned = aligned;
	}

	/* update the current break-address to match the requested address */
	pBrk.current = addr;
	return pBrk.current;
}
uint64_t sys::detail::Syscall::fHandleUName(env::guest_t addr) const {
	/* Note: memory errors will be handled automatically */
	logger.debug(u8"Syscall uname(", str::As{ U"#018x", addr }, u8") - Assumption: Entries are [sysname, nodename, release, version, machine] with each 65 chars");

	/* validate the address */
	if (addr == 0)
		return errCode::eFault;

	/* write the systemname out */
	env::Instance()->memory().mwrite(addr + 0 * 65llu, u8"wasmlator", 10, env::Usage::Write);

	/* write the nodename out */
	env::Instance()->memory().mwrite(addr + 1 * 65llu, u8"", 1, env::Usage::Write);

	/* write the release out */
	env::Instance()->memory().mwrite(addr + 2 * 65llu, u8"1.0.0-wasmlator", 16, env::Usage::Write);

	/* write the version out */
	env::Instance()->memory().mwrite(addr + 3 * 65llu, u8"1.0.0-userspace-wasmlator", 26, env::Usage::Write);

	/* write the machine out */
	env::Instance()->memory().mwrite(addr + 4 * 65llu, u8"wasm", 5, env::Usage::Write);
	return errCode::eSuccess;
}
uint64_t sys::detail::Syscall::fHandleOpenAt(int64_t dirfd, env::guest_t pathname, uint64_t flags, uint64_t mode) {
	logger.debug(u8"Syscall openat(", dirfd, u8", ", str::As{ U"#018x", pathname }, u8", ", flags, u8", ", mode, u8')');
	std::u8string path = fReadString(pathname);
	logger.debug(u8"pathname: [", path, u8"]");
	return pFileIO.openat(dirfd, path, flags, mode);
}

bool sys::detail::Syscall::setup(sys::Userspace* userspace, env::guest_t endOfData, std::u8string currentDirectory) {
	pUserspace = userspace;

	/* setup the syscall host with the handler id */
	pBrk = { endOfData, endOfData, endOfData };
	pPageSize = env::Instance()->pageSize();

	/* validate the alignment of the break */
	if ((pBrk.aligned & (pPageSize - 1)) != 0) {
		logger.error(u8"Initial break address [", str::As{ U"#018x", pBrk.aligned }, u8"] is not page aligned to [", str::As{ U"#010x", pPageSize }, u8']');
		return false;
	}

	/* setup the file-io */
	if (!pFileIO.setup(currentDirectory))
		return false;
	return true;
}
void sys::detail::Syscall::handle(env::guest_t address, env::guest_t nextAddress) {
	fHandle(address);
}
