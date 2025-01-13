#pragma once

#include <cinttypes>
#include <string>

namespace host {
	/* fetch a random 32-bit value */
	uint32_t Random();

	/* logs performed by the guest */
	void GuestStd(std::u8string_view msg);
	void GuestErr(std::u8string_view msg);
}
