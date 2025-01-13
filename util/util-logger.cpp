#include "util-logger.h"
#include "../host/interface.h"

static util::LogLevel GlobalLevel = util::LogLevel::trace;

void util::SetLogLevel(util::LogLevel level) {
	GlobalLevel = level;
}

util::LogLevel util::GetLogLevel() {
	return GlobalLevel;
}


util::Logger::Logger(std::u8string_view self) {
	if (!self.empty())
		pFormat = str::u8::Format(u8"[{: <16}] ", self);
}
void util::Logger::fLog(std::u8string_view msg, bool fatal) const {
	(fatal ? host_failure : host_message)(msg.data(), uint32_t(msg.size()));
}
