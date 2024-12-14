#include "interface.h"
#include "../interface/host.h"
#include "../environment/environment.h"
#include "../system/sys-primitive.h"
#include "../system/rv64/rv64-cpu.h"

static host::Logger logger{ u8"entry-point" };

void StartupProcess() {
	logger.log(u8"Main: Setting up system: [primitive] with cpu: [rv64]");
	env::Process::Create(sys::Primitive::New(
		rv64::Cpu::New(),
		{ u8"/this/path", u8"abc-def" },
		{ u8"abc=def", u8"ghi=jkl" }
	));
	logger.log(u8"Main: Process creation completed");
}
