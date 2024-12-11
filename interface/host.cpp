#include "host.h"
#include "../interface/interface.h"

static host::LogLevel GlobalLevel = host::LogLevel::trace;

void host::SetLogLevel(host::LogLevel level) {
	GlobalLevel = level;
}
host::LogLevel host::GetLogLevel() {
	return GlobalLevel;
}

void host::Log(const std::u8string_view& msg) {
	if (GlobalLevel >= host::LogLevel::log)
		host_print_u8(msg.data(), uint32_t(msg.size()));
}
void host::Debug(const std::u8string_view& msg) {
	if (GlobalLevel >= host::LogLevel::debug) {
		std::u8string buf;
		str::BuildTo(buf, u8"Debug: ", msg);
		host_print_u8(buf.data(), uint32_t(buf.size()));
	}
}
void host::Fatal(const std::u8string_view& msg) {
	host_fatal_u8(msg.data(), uint32_t(msg.size()));
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
