#include "host.h"
#include "../interface/interface.h"

static host::LogLevel GlobalLevel = host::LogLevel::debug;

void host::SetLogLevel(host::LogLevel level) {
	GlobalLevel = level;
}

void host::Log(const std::u8string_view& msg) {
	if (GlobalLevel != host::LogLevel::none)
		host_print_u8(msg.data(), uint32_t(msg.size()), 0);
}
void host::Debug(const std::u8string_view& msg) {
	if (GlobalLevel == host::LogLevel::debug) {
		std::u8string buf;
		str::BuildTo(buf, u8"Debug: ", msg);
		host_print_u8(buf.data(), uint32_t(buf.size()), 0);
	}
}
void host::Fatal(const std::u8string_view& msg) {
	std::u8string buf;
	str::BuildTo(buf, u8"Exception: ", msg);
	host_print_u8(buf.data(), uint32_t(buf.size()), true);
	host_abort();
}
