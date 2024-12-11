#include "host.h"
#include "../interface/interface.h"

static host::LogLevel GlobalLevel = host::LogLevel::trace;

void host::SetLogLevel(host::LogLevel level) {
	GlobalLevel = level;
}

host::LogLevel host::GetLogLevel() {
	return GlobalLevel;
}


host::Logger::Logger(std::u8string_view self) {
	pSelf = str::u8::Format(u8"{: <14}", self);
}
void host::Logger::fLog(std::u8string_view msg) const {
	host_print_u8(msg.data(), uint32_t(msg.size()));
}
void host::Logger::fFatal [[noreturn]] (std::u8string_view msg) const {
	host_fatal_u8(msg.data(), uint32_t(msg.size()));
}
