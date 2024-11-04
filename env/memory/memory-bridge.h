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
		static bool ExpandPhysical(uint32_t pages);
		static void MovePhysical(env::physical_t dest, env::physical_t source, uint32_t size);
		static void FlushCaches();

		static uint32_t Readi32Fromu8(env::guest_t address);
		static uint32_t Readi32Fromi8(env::guest_t address);
		static uint32_t Readi32Fromu16(env::guest_t address);
		static uint32_t Readi32Fromi16(env::guest_t address);
		static uint32_t Readi32(env::guest_t address);
		static uint64_t Readi64(env::guest_t address);
		static float Readf32(env::guest_t address);
		static double Readf64(env::guest_t address);

		static void Writei32Fromu8(env::guest_t address, uint32_t value);
		static void Writei32Fromi8(env::guest_t address, uint32_t value);
		static void Writei32Fromu16(env::guest_t address, uint32_t value);
		static void Writei32Fromi16(env::guest_t address, uint32_t value);
		static void Writei32(env::guest_t address, uint32_t value);
		static void Writei64(env::guest_t address, uint64_t value);
		static void Writef32(env::guest_t address, float value);
		static void Writef64(env::guest_t address, double value);

		static uint32_t Codei32Fromu8(env::guest_t address);
		static uint32_t Codei32Fromi8(env::guest_t address);
		static uint32_t Codei32Fromu16(env::guest_t address);
		static uint32_t Codei32Fromi16(env::guest_t address);
		static uint32_t Codei32(env::guest_t address);
		static uint64_t Codei64(env::guest_t address);
		static float Codef32(env::guest_t address);
		static double Codef64(env::guest_t address);
	};
}
