#pragma once

#include "sys-elf-types.h"
#include "sys-static-elf.h"

namespace sys::elf {
	/* validate the initial elf-header */
	bool CheckValid(const uint8_t* data, size_t size);

	/* fetch the elf-type from the given bytes or return none, if it does not exist */
	elf::ElfType GetType(const uint8_t* data, size_t size);

	/* fetch the elf-machine type from the given bytes or return none, if it does not exist */
	elf::MachineType GetMachine(const uint8_t* data, size_t size);

	/* fetch the elf bit-width from the given bytes or return 0, if it does not exist */
	uint32_t GetBitWidth(const uint8_t* data, size_t size);
}
