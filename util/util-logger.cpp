#include "util-logger.h"
#include "../host/interface.h"

static bool GlobalLogEnabled = true;

void util::ConfigureLogging(bool enabled) {
	GlobalLogEnabled = enabled;
}

bool util::LoggingEnabled() {
	return GlobalLogEnabled;
}


util::Logger::Logger(std::u8string_view self) {
	pFormat = str::u8::Format(u8"[{: <16}] ", self);
}
void util::Logger::fLog(std::u8string_view msg, bool fatal) const {
	(fatal ? host_failure : host_message)(msg.data(), uint32_t(msg.size()));
}
