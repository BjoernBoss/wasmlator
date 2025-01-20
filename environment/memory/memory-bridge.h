#pragma once

#include "../env-common.h"

namespace env::detail {
	struct MemoryBridge {
		/* exports */
		static void Lookup(uint64_t address, uint64_t access, uint32_t size, uint32_t usage, uint32_t cache);

		/* imports */
		static bool ExpandPhysical(uint64_t pages);
		static void MovePhysical(uint64_t dest, uint64_t source, uint64_t size);
		static void WriteToPhysical(uint64_t dest, const void* source, uint64_t size);
		static void ReadFromPhysical(void* dest, uint64_t source, uint64_t size);
		static void ClearPhysical(uint64_t dest, uint64_t size);
		static uint64_t Read(env::guest_t address, uint64_t size);
		static void Write(env::guest_t address, uint64_t size, uint64_t value);
		static uint64_t Code(env::guest_t address, uint64_t size);
	};
}
