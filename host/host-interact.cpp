#include "host-interact.h"
#include "../host/interface.h"

uint32_t host::Random() {
	return host_random();
}

uint64_t host::GetStampUS() {
	return host_time_us();
}

void host::GuestOut(std::u8string_view msg) {
	host_guest_out(msg.data(), uint32_t(msg.size()));
}
