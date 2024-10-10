#include "logging.h"
#include "../interface/interface.h"

void util::log(const std::u8string_view& msg) {
	host_print_u8(msg.data(), uint32_t(msg.size()));
}
void util::fail(const std::u8string_view& msg) {
	host_fail_u8(msg.data(), uint32_t(msg.size()));
}
