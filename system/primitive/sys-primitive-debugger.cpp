#include "../system.h"

static host::Logger logger{ u8"sys::primitive" };

sys::detail::PrimitiveDebugger::PrimitiveDebugger(sys::Primitive* primitive) : pPrimitive{ primitive } {}
std::unique_ptr<sys::Debuggable> sys::detail::PrimitiveDebugger::New(sys::Primitive* primitive) {
	return std::unique_ptr<detail::PrimitiveDebugger>(new detail::PrimitiveDebugger{ primitive });
}
void sys::detail::PrimitiveDebugger::getState(std::unordered_map<std::u8string, uintptr_t>& state) const {

}
env::guest_t sys::detail::PrimitiveDebugger::getPC() const {
	return pPrimitive->pAddress;
}
std::u8string sys::detail::PrimitiveDebugger::decode(uintptr_t address) const {
	return u8"";
}
void sys::detail::PrimitiveDebugger::setValue(std::u8string_view name, uintptr_t value) {

}
void sys::detail::PrimitiveDebugger::setPC(env::guest_t pc) {
	pPrimitive->pAddress = pc;
}
void sys::detail::PrimitiveDebugger::run() {
	/* execute will internally check the debugger-completed state */
	pPrimitive->fExecute();
}
