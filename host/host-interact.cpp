#include "host-interact.h"
#include "../host/interface.h"

uint32_t host::Random() {
	return host_random();
}

uint64_t host::GetStampUS() {
	return host_time_us();
}

int32_t host::GetTimeZoneMIN() {
	return host_timezone_min();
}

void host::PrintOut(std::u8string_view msg) {
	std::u8string actual = str::u8::Build(u8"O:", msg);
	host_message(actual.data(), uint32_t(actual.size()));
}

void host::PrintOutLn(std::u8string_view msg) {
	std::u8string actual = str::u8::Build(u8"O:", msg, u8'\n');
	host_message(actual.data(), uint32_t(actual.size()));
}
