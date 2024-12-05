#pragma once

#include "elf-common.h"

namespace elf {
	/* load static elf file to current env::Process and return the entry-point address */
	env::guest_t LoadStatic(const uint8_t* data, size_t size);
}
