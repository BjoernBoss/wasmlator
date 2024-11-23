#include "interface.h"
#include "../interface/host.h"
#include "../environment/environment.h"
#include "../system/sys-primitive.h"
#include "../system/rv64/rv64-cpu.h"

void StartupProcess() {
	host::Log(u8"Main: Application startup entered");

	host::Debug(u8"Main: Setting up system: [primitive] with cpu: [rv64]");
	env::Process::Create(sys::Primitive::New(rv64::Cpu::New()));

	host::Log(u8"Main: Application startup exited");
}
