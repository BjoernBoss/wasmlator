#include "../system.h"

static host::Logger logger{ u8"sys::primitive" };

sys::detail::PrimitiveExecContext::PrimitiveExecContext(sys::Primitive* primitive) : sys::ExecContext{ false, true }, pPrimitive{ primitive } {}

void sys::detail::PrimitiveExecContext::fHandleSyscall() {
	sys::UserSpaceSyscall call = pPrimitive->pCpu->getSyscallArgs();

	/* check if the syscall can be handled in-place */
	switch (call.index) {
	case sys::SyscallIndex::read:
		logger.log(u8"Syscall read invoked");
		break;
	case sys::SyscallIndex::write:
		logger.log(u8"Syscall write invoked");
		break;
	case sys::SyscallIndex::unknown:
		throw detail::UnknownSyscall{ pSyscall.address, call.rawIndex };
		break;
	}

	/* write the result back */
	pPrimitive->pCpu->setSyscallResult(uint64_t(-1));
}

std::unique_ptr<sys::detail::PrimitiveExecContext> sys::detail::PrimitiveExecContext::New(sys::Primitive* primitive) {
	return std::unique_ptr<detail::PrimitiveExecContext>(new detail::PrimitiveExecContext{ primitive });
}

void sys::detail::PrimitiveExecContext::syscall(env::guest_t address, env::guest_t nextAddress) {
	/* cache the current address and next address */
	gen::Add[I::U64::Const(address)];
	gen::Make->writeHost(&pSyscall.address, gen::MemoryType::i64);
	gen::Add[I::U64::Const(nextAddress)];
	gen::Make->writeHost(&pSyscall.next, gen::MemoryType::i64);

	/* perform the actual syscall */
	gen::Make->invokeVoid(pRegistered.syscall);
}
void sys::detail::PrimitiveExecContext::throwException(uint64_t id, env::guest_t address, env::guest_t nextAddress) {
	/* write the id to the cache */
	gen::Add[I::U64::Const(id)];
	gen::Make->writeHost(&pException.id, gen::MemoryType::i64);

	/* write the address to the cache */
	gen::Add[I::U64::Const(address)];
	gen::Make->writeHost(&pException.address, gen::MemoryType::i64);

	/* perform the exception-call */
	gen::Make->invokeVoid(pRegistered.exception);
}
void sys::detail::PrimitiveExecContext::flushMemCache(env::guest_t address, env::guest_t nextAddress) {
	/* nothing to be done here, as the system is considered single-threaded */
}
void sys::detail::PrimitiveExecContext::flushInstCache(env::guest_t address, env::guest_t nextAddress) {
	gen::Add[I::U64::Const(nextAddress)];
	gen::Make->invokeParam(pRegistered.flushInst);
	gen::Add[I::Drop()];
}

bool sys::detail::PrimitiveExecContext::coreLoaded() {
	/* register the functions to be invoked by the execution-environment */
	pRegistered.flushInst = env::Instance()->interact().defineCallback([](uint64_t address) -> uint64_t {
		throw detail::FlushInstCache{ address };
		return 0;
		});
	pRegistered.exception = env::Instance()->interact().defineCallback([&]() {
		throw detail::CpuException{ pException.address, pException.id };
		});
	pRegistered.syscall = env::Instance()->interact().defineCallback([&]() {
		fHandleSyscall();
		});
	return true;
}
