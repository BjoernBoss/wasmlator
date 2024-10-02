#pragma once

#include "env-memory.h"

namespace env::bridge {
	struct Memory {
		struct LookupOutput {
			uint64_t address;
			uint32_t offset;
			uint32_t size;
		};

		static LookupOutput Lookup(uint64_t self, uint64_t address, uint32_t size, uint32_t usage);

		static uint32_t Readi32Fromu8(env::id_t id, env::addr_t address);
		static uint32_t Readi32Fromi8(env::id_t id, env::addr_t address);
		static uint32_t Readi32Fromu16(env::id_t id, env::addr_t address);
		static uint32_t Readi32Fromi16(env::id_t id, env::addr_t address);
		static uint32_t Readi32(env::id_t id, env::addr_t address);
		static uint64_t Readi64(env::id_t id, env::addr_t address);
		static float Readf32(env::id_t id, env::addr_t address);
		static double Readf64(env::id_t id, env::addr_t address);

		static void Writei32Fromu8(env::id_t id, env::addr_t address, uint32_t value);
		static void Writei32Fromi8(env::id_t id, env::addr_t address, uint32_t value);
		static void Writei32Fromu16(env::id_t id, env::addr_t address, uint32_t value);
		static void Writei32Fromi16(env::id_t id, env::addr_t address, uint32_t value);
		static void Writei32(env::id_t id, env::addr_t address, uint32_t value);
		static void Writei64(env::id_t id, env::addr_t address, uint64_t value);
		static void Writef32(env::id_t id, env::addr_t address, float value);
		static void Writef64(env::id_t id, env::addr_t address, double value);

		static uint32_t Executei32Fromu8(env::id_t id, env::addr_t address);
		static uint32_t Executei32Fromi8(env::id_t id, env::addr_t address);
		static uint32_t Executei32Fromu16(env::id_t id, env::addr_t address);
		static uint32_t Executei32Fromi16(env::id_t id, env::addr_t address);
		static uint32_t Executei32(env::id_t id, env::addr_t address);
		static uint64_t Executei64(env::id_t id, env::addr_t address);
		static float Executef32(env::id_t id, env::addr_t address);
		static double Executef64(env::id_t id, env::addr_t address);
	};
}
