#include "../system.h"

static host::Logger logger{ u8"sys::primitive" };

sys::detail::PrimitiveDebugger::PrimitiveDebugger(sys::Primitive* primitive) : pPrimitive{ primitive } {}

std::unique_ptr<sys::Debuggable> sys::detail::PrimitiveDebugger::New(sys::Primitive* primitive) {
	return std::unique_ptr<detail::PrimitiveDebugger>(new detail::PrimitiveDebugger{ primitive });
}

sys::Cpu* sys::detail::PrimitiveDebugger::getCpu() const {
	return pPrimitive->pCpu;
}
env::guest_t sys::detail::PrimitiveDebugger::getPC() const {
	return pPrimitive->pAddress;
}
void sys::detail::PrimitiveDebugger::setPC(env::guest_t pc) {
	pPrimitive->pAddress = pc;
}
void sys::detail::PrimitiveDebugger::run() {
	/* execute will internally check the debugger-completed state */
	pPrimitive->fExecute();
}
