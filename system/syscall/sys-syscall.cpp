#include "sys-syscall.h"

static host::Logger logger{ u8"sys::Syscall" };

void sys::Syscall::fHandle() {
	sys::SyscallArgs call = pCpu->getArgs();

	/* check if the syscall can be handled in-place */
	switch (call.index) {
	case sys::SyscallIndex::read:
		logger.fatal(u8"Syscall [read] not implemented");
		break;
	case sys::SyscallIndex::write:
		logger.fatal(u8"Syscall [write] not implemented");
		break;
	case sys::SyscallIndex::getuid:
		pCpu->setResult(fHandleId(u8"getuid"));
		return;
	case sys::SyscallIndex::geteuid:
		pCpu->setResult(fHandleId(u8"geteuid"));
		return;
	case sys::SyscallIndex::getgid:
		pCpu->setResult(fHandleId(u8"getgid"));
		return;
	case sys::SyscallIndex::getegid:
		pCpu->setResult(fHandleId(u8"getegid"));
		return;
	case sys::SyscallIndex::brk:
		pCpu->setResult(fHandleBrk(call.args[0]));
		break;
	case sys::SyscallIndex::uname:
		fHandleUName(call.args[0]);
		break;
	case sys::SyscallIndex::unknown:
		throw sys::UnknownSyscall{ pActive.address, call.rawIndex };
		break;
	default:
		logger.fatal(u8"Syscall currently not implemented");
		break;
	}
}
uint64_t sys::Syscall::fHandleId(std::u8string_view name) const {
	logger.debug(u8"Syscall [", name, u8"] = ", env::Instance()->getId());
	return uint16_t(env::Instance()->getId());
}
env::guest_t sys::Syscall::fHandleBrk(env::guest_t addr) {
	logger.debug(u8"Syscall [brk](", str::As{ U"#018x", addr }, u8')');

	/* check if the address lies beneath the initial address, in which case
	*	the current break can just be returned, as no changes will be made */
	if (addr < pBrk.init) {
		logger.debug(u8"result: ", str::As{ U"#018x", pBrk.current });
		return pBrk.current;
	}

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
			logger.debug(u8"result: ", str::As{ U"#018x", pBrk.current });
			return pBrk.current;
		}
		pBrk.aligned = aligned;
	}

	/* update the current break-address to match the requested address */
	pBrk.current = addr;
	logger.debug(u8"result: ", str::As{ U"#018x", pBrk.current });
	return pBrk.current;
}
uint64_t sys::Syscall::fHandleUName(env::guest_t addr) const {
	logger.debug(u8"Syscall [uname](", str::As{ U"#018x", addr }, u8") - Assumption: Entries are [sysname, nodename, release, version, machine] with each 65 chars");

	/* validate the address */
	if (addr == 0) {
		logger.debug(u8"result: ", errCode::eFault);
		return errCode::eFault;
	}

	try {
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
	}
	catch (const env::MemoryFault& e) {
		logger.debug(u8"result: ", errCode::eFault);
		return errCode::eFault;
	}

	/* return the successful write */
	logger.debug(u8"result: ", errCode::eSuccess);
	return errCode::eSuccess;
}

std::unique_ptr<sys::Syscall> sys::Syscall::New(std::unique_ptr<sys::Syscallable> provider, env::guest_t endOfData) {
	std::unique_ptr<sys::Syscall> syscall{ new sys::Syscall() };

	/* setup the syscall host with the handler id */
	syscall->pProvider = std::move(provider);
	syscall->pCpu = syscall->pProvider->getCpu();
	syscall->pBrk = { endOfData, endOfData, endOfData };
	syscall->pPageSize = env::Instance()->pageSize();
	if (syscall->pCpu.get() == 0)
		return 0;

	/* validate the alignment of the break */
	if ((syscall->pBrk.aligned & (syscall->pPageSize - 1)) != 0) {
		logger.fatal(u8"Initial break address [", str::As{ U"#018x", syscall->pBrk.aligned }, u8"] is not page aligned to [", str::As{ U"#010x", syscall->pPageSize }, u8']');
		return 0;
	}

	/* register the callback-function to the syscall-handler */
	syscall->pSyscallHandlerId = env::Instance()->interact().defineCallback([_this = syscall.get()]() {
		_this->fHandle();
		});

	return syscall;
}
void sys::Syscall::makeSyscall(env::guest_t address, env::guest_t nextAddress) {
	/* cache the current address and next address */
	gen::Add[I::U64::Const(address)];
	gen::Make->writeHost(&pActive.address, gen::MemoryType::i64);
	gen::Add[I::U64::Const(nextAddress)];
	gen::Make->writeHost(&pActive.next, gen::MemoryType::i64);

	/* perform the actual syscall */
	gen::Make->invokeVoid(pSyscallHandlerId);
}
