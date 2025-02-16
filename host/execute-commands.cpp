#include <arger/arger.h>
#include "../host/interface.h"
#include "../system/system.h"
#include "../rv64/rv64-cpu.h"

void ExecuteCommand(std::u8string_view cmd) {
	util::ConfigureLogging(false);

	/* echo the input command back */
	host::PrintOut(str::u8::Build(u8"> ", cmd));

	/* check if an execution has already been set up and not cleaned up properly */
	if (env::Instance() != 0) {
		host::PrintOut(u8"Process is already loaded.");
		return;
	}

	/* parse the command-line arguments (ensure that they are not empty) */
	std::vector<std::wstring> split = arger::Prepare(cmd);

	/* extract the binary and the arguments */
	std::u8string binary;
	std::vector<std::u8string> args;
	binary = (split.empty() ? u8"" : str::u8::To(split[0]));
	for (size_t i = 1; i < split.size(); ++i)
		args.push_back(str::u8::To(split[i]));

	/* try to setup the userspace system */
	if (!sys::Userspace::Create(rv64::Cpu::New(), binary, args, {}, false, gen::TraceType::none, 0))
		host::PrintOut(u8"Failed to create process");
}
void CleanupExecute() {
	/* shutdown the system */
	if (env::Instance() != 0)
		env::Instance()->shutdown();
}
