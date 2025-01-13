#include "host-interact.h"
#include "../host/interface.h"

uint32_t host::Random() {
	return host_random();
}

void host::GuestStd(std::u8string_view msg) {
	host_stdout(msg.data(), uint32_t(msg.size()));
}
void host::GuestErr(std::u8string_view msg) {
	host_stderr(msg.data(), uint32_t(msg.size()));
}
