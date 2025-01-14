#pragma once

#include <cinttypes>
#include <string>

namespace host {
	/* fetch a random 32-bit value */
	uint32_t Random();

	/* fetch the current time in [us] */
	uint64_t GetStampUS();

	/* logs performed by the guest */
	void GuestOut(std::u8string_view msg);
}
