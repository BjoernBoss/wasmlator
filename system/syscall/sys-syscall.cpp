#include "sys-syscall.h"

static host::Logger logger{ u8"sys::Syscall" };

void sys::Syscall::fHandle() {
	sys::SyscallArgs call = pProvider->getArgs();

	/* check if the syscall can be handled in-place */
	switch (call.index) {
	case sys::SyscallIndex::read:
		logger.fatal(u8"Syscall [read] not implemented");
		break;
	case sys::SyscallIndex::write:
		logger.fatal(u8"Syscall [write] not implemented");
		break;
	case sys::SyscallIndex::getuid:
		logger.debug(u8"Syscall [getuid]");
		pProvider->setResult(uint16_t(env::Instance()->getId()));
		return;
	case sys::SyscallIndex::geteuid:
		logger.debug(u8"Syscall [geteuid]");
		pProvider->setResult(uint16_t(env::Instance()->getId()));
		return;
	case sys::SyscallIndex::getgid:
		logger.debug(u8"Syscall [getgid]");
		pProvider->setResult(uint16_t(env::Instance()->getId()));
		return;
	case sys::SyscallIndex::getegid:
		logger.debug(u8"Syscall [getegid]");
		pProvider->setResult(uint16_t(env::Instance()->getId()));
		return;
	case sys::SyscallIndex::brk:
		logger.debug(u8"Syscall [brk]");
		break;
	case sys::SyscallIndex::unknown:
		throw sys::UnknownSyscall{ pActive.address, call.rawIndex };
		break;
	default:
		logger.fatal(u8"Unknown syscall index encountered");
		break;
	}
}

std::unique_ptr<sys::Syscall> sys::Syscall::New(std::unique_ptr<sys::Syscallable> provider) {
	std::unique_ptr<sys::Syscall> syscall{ new sys::Syscall() };

	/* setup the syscall host with the handler id */
	syscall->pProvider = std::move(provider);
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
