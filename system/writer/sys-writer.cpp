#include "../system.h"

bool sys::Writer::fSetup(detail::Syscall* syscall) {
	/* register the functions to be invoked by the execution-environment */
	pRegistered.flushInst = env::Instance()->interact().defineCallback([](uint64_t address) -> uint64_t {
		throw detail::FlushInstCache{ address };
		return 0;
		});
	pRegistered.exception = env::Instance()->interact().defineCallback([this]() {
		throw detail::CpuException{ pException.address, pException.id };
		});
	pRegistered.syscall = env::Instance()->interact().defineCallback([syscall, this]() {
		syscall->handle(pSyscall.address, pSyscall.next);
		});
	return true;
}

void sys::Writer::makeFlushMemCache(env::guest_t address, env::guest_t nextAddress) const {
	/* nothing to be done here, as the system is considered single-threaded */
}
void sys::Writer::makeFlushInstCache(env::guest_t address, env::guest_t nextAddress) const {
	gen::Add[I::U64::Const(nextAddress)];
	gen::Make->invokeParam(pRegistered.flushInst);
	gen::Add[I::Drop()];
}
void sys::Writer::makeSyscall(env::guest_t address, env::guest_t nextAddress) {
	/* write the address to the cache */
	gen::Add[I::U64::Const(address)];
	gen::Make->writeHost(&pSyscall.address, gen::MemoryType::i64);

	/* write the next address to the cache */
	gen::Add[I::U64::Const(nextAddress)];
	gen::Make->writeHost(&pSyscall.next, gen::MemoryType::i64);

	/* perform the syscall-call */
	gen::Make->invokeVoid(pRegistered.syscall);

}
void sys::Writer::makeException(uint64_t id, env::guest_t address, env::guest_t nextAddress) {
	/* write the id to the cache */
	gen::Add[I::U64::Const(id)];
	gen::Make->writeHost(&pException.id, gen::MemoryType::i64);

	/* write the address to the cache */
	gen::Add[I::U64::Const(address)];
	gen::Make->writeHost(&pException.address, gen::MemoryType::i64);

	/* perform the exception-call */
	gen::Make->invokeVoid(pRegistered.exception);
}
