#include "sys-primitive.h"

static host::Logger logger{ u8"sys::primitive" };

sys::detail::PrimitiveSyscall::PrimitiveSyscall(sys::Primitive* primitive) : pPrimitive{ primitive } {}

std::unique_ptr<sys::detail::PrimitiveSyscall> sys::detail::PrimitiveSyscall::New(sys::Primitive* primitive) {
	return std::unique_ptr<detail::PrimitiveSyscall>(new detail::PrimitiveSyscall{ primitive });
}

std::unique_ptr<sys::CpuSyscallable> sys::detail::PrimitiveSyscall::getCpu() {
	return pPrimitive->pCpu->getSyscall();
}
void sys::detail::PrimitiveSyscall::run(env::guest_t address) {
	pPrimitive->pAddress = address;
	pPrimitive->fExecute();
}
