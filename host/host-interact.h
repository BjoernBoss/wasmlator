#pragma once

#include <cinttypes>
#include <string>
#include <ustring/ustring.h>

namespace host {
	/* fetch a random 32-bit value */
	uint32_t Random();

	/* fetch the current time in [us] */
	uint64_t GetStampUS();

	/* fetch the current timezone east of utc in [min] */
	int32_t GetTimeZoneMIN();

	/* direct logs out without logging wrapper */
	void PrintOut(std::u8string_view msg);
}
