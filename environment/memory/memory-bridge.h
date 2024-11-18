#pragma once

#include "../env-common.h"

namespace env::detail {
	struct MemoryBridge {
		/* exports */
		static void Lookup(uint64_t address, uint32_t size, uint32_t usage);
		static uint64_t LookupAddress();
		static uint32_t LookupSize();
		static uint32_t LookupPhysical();

		/* imports */
		static void FlushCaches();
		static bool ExpandPhysical(uint32_t pages);
		static void MovePhysical(detail::physical_t dest, detail::physical_t source, uint32_t size);
		static void WriteToPhysical(detail::physical_t dest, const uint8_t* source, uint32_t size);
		static void ReadFromPhysical(uint8_t* dest, detail::physical_t source, uint32_t size);
		static uint64_t Read(env::guest_t address, uint32_t size);
		static void Write(env::guest_t address, uint32_t size, uint64_t value);
		static uint64_t Code(env::guest_t address, uint32_t size);
	};
}
