#include "host-logger.h"
#include "../host/interface.h"

static host::LogLevel GlobalLevel = host::LogLevel::trace;

void host::SetLogLevel(host::LogLevel level) {
	GlobalLevel = level;
}

host::LogLevel host::GetLogLevel() {
	return GlobalLevel;
}


host::Logger::Logger(std::u8string_view self) {
	if (!self.empty())
		pFormat = str::u8::Format(u8"[{: <16}] ", self);
}
void host::Logger::fLog(std::u8string_view msg, bool fatal) const {
	host_print_u8(msg.data(), uint32_t(msg.size()), fatal);
}
