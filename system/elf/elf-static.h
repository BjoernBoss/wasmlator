#pragma once

#include "elf-common.h"

namespace elf {
	struct Output {
		env::guest_t start = 0;
		env::guest_t endOfData = 0;
	};

	/* load static elf file to current env::Process */
	elf::Output LoadStatic(const uint8_t* data, size_t size);
}
