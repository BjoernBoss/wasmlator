/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025-2026 Bjoern Boss Henrichsen */
#include <arger/arger.h>
#include "../host/interface.h"
#include "../system/system.h"
#include "../rv64/rv64-cpu.h"

void ExecuteCommand(std::u8string_view cmd) {
	util::ConfigureLogging(false);

	/* check if an execution has already been set up and not cleaned up properly */
	if (env::Instance() != 0) {
		host::PrintOutLn(u8"Process is already loaded.");
		return;
	}

	/* parse the command-line arguments (ensure that they are not empty) */
	std::vector<std::string> split = arger::Prepare(cmd);

	/* extract the binary and the arguments to setup the new userspace configuration */
	sys::RunConfig config;
	config.binary = (split.empty() ? u8"" : str::u8::To(split[0]));
	for (size_t i = 1; i < split.size(); ++i)
		config.args.push_back(str::u8::To(split[i]));

	/* try to setup the userspace system */
	if (!sys::Userspace::Create(rv64::Cpu::New(), config))
		host::PrintOutLn(u8"Failed to create process.");
}

void CleanupExecute() {
	/* shutdown the system */
	if (env::Instance() != 0)
		env::Instance()->shutdown();
}
