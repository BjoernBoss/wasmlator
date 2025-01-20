#pragma once

#include "sys-elf-types.h"

namespace sys {
	struct ElfLoaded {
		env::guest_t start = 0;
		env::guest_t endOfData = 0;
		env::guest_t phAddress = 0;
		uint32_t phEntrySize = 0;
		uint32_t phCount = 0;
	};

	/* load static elf file to current env::Process */
	//sys::ElfLoaded LoadElfStatic(const uint8_t* data, size_t size);
}
