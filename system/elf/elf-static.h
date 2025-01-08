#pragma once

#include "elf-common.h"

namespace elf {
	struct Output {
		env::guest_t start = 0;
		env::guest_t endOfData = 0;
		env::guest_t phAddress = 0;
		uint32_t phEntrySize = 0;
		uint32_t phCount = 0;
		bool is64Bit = false;
	};

	/* load static elf file to current env::Process */
	elf::Output LoadStatic(const uint8_t* data, size_t size);
}
