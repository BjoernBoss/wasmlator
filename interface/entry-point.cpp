#include "interface.h"
#include "../interface/host.h"
#include "../environment/env-process.h"
#include "../system/rv64/rv64-specification.h"

void StartupProcess() {
	host::Log(u8"Main: Application startup entered");
	env::Process::Create(rv64::Specification::New(0x1234));

	host::Log(u8"Main: Application startup exited");
}
