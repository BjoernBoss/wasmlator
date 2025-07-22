/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025 Bjoern Boss Henrichsen */
#include "../util/util-logger.h"
#include "../host/interface.h"

int main() {
	util::ConfigureLogging(true);

	while (true) {
		std::string cmd;
		std::getline(std::cin, cmd);
		HandleCommand(str::u8::Safe(cmd));
	}
	return 0;
}
