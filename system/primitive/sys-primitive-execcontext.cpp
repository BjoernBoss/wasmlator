#include "sys-primitive.h"

static host::Logger logger{ u8"sys::primitive" };

sys::detail::PrimitiveExecContext::PrimitiveExecContext(sys::Primitive* primitive) : sys::ExecContext{ false, true }, pPrimitive{ primitive } {}

std::unique_ptr<sys::detail::PrimitiveExecContext> sys::detail::PrimitiveExecContext::New(sys::Primitive* primitive) {
	return std::unique_ptr<detail::PrimitiveExecContext>(new detail::PrimitiveExecContext{ primitive });
}

void sys::detail::PrimitiveExecContext::syscall(env::guest_t address, env::guest_t nextAddress) {
	pSyscall->makeSyscall(address, nextAddress);
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
bool sys::detail::PrimitiveExecContext::coreLoaded(env::guest_t endOfData) {
	/* register the functions to be invoked by the execution-environment */
	pRegistered.flushInst = env::Instance()->interact().defineCallback([](uint64_t address) -> uint64_t {
		throw detail::FlushInstCache{ address };
		return 0;
		});
	pRegistered.exception = env::Instance()->interact().defineCallback([&]() {
		throw detail::CpuException{ pException.address, pException.id };
		});

	/* setup the syscall handler */
	pSyscall = sys::Syscall::New(detail::PrimitiveSyscall::New(pPrimitive), endOfData);
	if (pSyscall.get() == 0)
		logger.error(u8"Failed to attach syscall handler (supported by cpu?)");
	return (pSyscall.get() != 0);
}
