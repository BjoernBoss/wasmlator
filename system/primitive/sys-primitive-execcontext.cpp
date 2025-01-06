#include "../system.h"

static host::Logger logger{ u8"sys::primitive" };

sys::detail::PrimitiveExecContext::PrimitiveExecContext(sys::Primitive* primitive) : sys::ExecContext{ false, true }, pPrimitive{ primitive } {}
std::unique_ptr<sys::ExecContext> sys::detail::PrimitiveExecContext::New(sys::Primitive* primitive) {
	return std::unique_ptr<detail::PrimitiveExecContext>(new detail::PrimitiveExecContext{ primitive });
}
void sys::detail::PrimitiveExecContext::syscall(env::guest_t address, env::guest_t nextAddress) {
	gen::Make->invokeVoid(pPrimitive->pRegistered.syscall);
}
void sys::detail::PrimitiveExecContext::throwException(uint64_t id, env::guest_t address, env::guest_t nextAddress) {
	gen::Make->invokeVoid(pPrimitive->pRegistered.exception);
}
void sys::detail::PrimitiveExecContext::flushMemCache(env::guest_t address, env::guest_t nextAddress) {
	/* nothing to be done here, as the system is considered single-threaded */
}
void sys::detail::PrimitiveExecContext::flushInstCache(env::guest_t address, env::guest_t nextAddress) {
	gen::Add[I::U64::Const(pPrimitive->pRegistered.flushInst)];
	gen::Make->invokeParam(pPrimitive->pRegistered.flushInst);
	gen::Add[I::Drop()];
}
