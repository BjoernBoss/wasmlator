/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025 Bjoern Boss Henrichsen */
#include "util-logger.h"
#include "../host/interface.h"

static bool GlobalLogEnabled = true;

void util::ConfigureLogging(bool enabled) {
	GlobalLogEnabled = enabled;
}

bool util::LoggingEnabled() {
	return GlobalLogEnabled;
}


util::Logger util::detail::LoggerAccess::MakeNullLogger() {
	return util::Logger{};
}

util::Logger::Logger() {}
util::Logger::Logger(std::u8string_view self) {
	if (!self.empty())
		pFormat = str::u8::Format(u8"[{: <16}] ", self);
}
void util::Logger::fLog(std::u8string_view msg, bool fatal) const {
	(fatal ? host_failure : host_message)(msg.data(), uint32_t(msg.size()));
}

/* null-logger instance */
util::Logger util::nullLogger = util::detail::LoggerAccess::MakeNullLogger();
