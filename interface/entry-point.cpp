#include "../interface/interface.h"
#include "../interface/host.h"
#include "../environment/environment.h"
#include "../system/sys-primitive.h"
#include "../system/rv64/rv64-cpu.h"

static host::Logger logger{ u8"dispatch" };

void StartupProcess() {
	logger.log(u8"StartupProcess");
}

static bool DispatchCommand(std::u8string_view cmd) {
	if (cmd == u8"setup") {
		logger.log(u8"Setting up system: [primitive] with cpu: [rv64]");
		sys::Primitive::Create(rv64::Cpu::New(),
			{ u8"/this/path", u8"abc-def" },
			{ u8"abc=def", u8"ghi=jkl" },
			true);
		logger.log(u8"Process creation completed");
		return true;
	}
	else if (cmd == u8"shutdown") {
		env::Instance()->system()->shutdown();
		return true;
	}

	return false;
}

void HandleCommand(std::u8string_view cmd) {
	if (cmd.empty() || !DispatchCommand(cmd))
		logger.warn(u8"Unknown command [", cmd, u8"] received");
}
