#include "memory-bridge.h"

extern "C" static env::bridge::Memory::LookupOutput MemoryPerformLookup(env::bridge::Memory::LookupInput input) {
	return env::bridge::Memory::Lookup(input);
}

env::bridge::Memory::LookupOutput env::bridge::Memory::Lookup(Memory::LookupInput input) {
	env::MemoryRegion reg = reinterpret_cast<env::Memory*>(input.memory)->fLookup(input.address, input.size, uint32_t(input.usage));
	return { reg.address, reg.physical, reg.size };
}

#ifdef EMSCRIPTEN_COMPILATION

extern "C" {
	static uint32_t jsMemoryReadi32Fromu8(uint32_t id, uint64_t address);
	static uint32_t jsMemoryReadi32Fromi8(uint32_t id, uint64_t address);
	static uint32_t jsMemoryReadi32Fromu16(uint32_t id, uint64_t address);
	static uint32_t jsMemoryReadi32Fromi16(uint32_t id, uint64_t address);
	static uint32_t jsMemoryReadi32(uint32_t id, uint64_t address);
	static uint64_t jsMemoryReadi64(uint32_t id, uint64_t address);
	static float jsMemoryReadf32(uint32_t id, uint64_t address);
	static double jsMemoryReadf64(uint32_t id, uint64_t address);
	static void jsMemoryWritei32Fromu8(uint32_t id, uint64_t address, uint32_t value);
	static void jsMemoryWritei32Fromi8(uint32_t id, uint64_t address, uint32_t value);
	static void jsMemoryWritei32Fromu16(uint32_t id, uint64_t address, uint32_t value);
	static void jsMemoryWritei32Fromi16(uint32_t id, uint64_t address, uint32_t value);
	static void jsMemoryWritei32(uint32_t id, uint64_t address, uint32_t value);
	static void jsMemoryWritei64(uint32_t id, uint64_t address, uint64_t value);
	static void jsMemoryWritef32(uint32_t id, uint64_t address, float value);
	static void jsMemoryWritef64(uint32_t id, uint64_t address, double value);
	static uint32_t jsMemoryExecutei32Fromu8(uint32_t id, uint64_t address);
	static uint32_t jsMemoryExecutei32Fromi8(uint32_t id, uint64_t address);
	static uint32_t jsMemoryExecutei32Fromu16(uint32_t id, uint64_t address);
	static uint32_t jsMemoryExecutei32Fromi16(uint32_t id, uint64_t address);
	static uint32_t jsMemoryExecutei32(uint32_t id, uint64_t address);
	static uint64_t jsMemoryExecutei64(uint32_t id, uint64_t address);
	static float jsMemoryExecutef32(uint32_t id, uint64_t address);
	static double jsMemoryExecutef64(uint32_t id, uint64_t address);
}

uint32_t env::bridge::Memory::Readi32Fromu8(env::environment_t id, env::addr_t address) {
	return jsMemoryReadi32Fromu8(id, address);
}
uint32_t env::bridge::Memory::Readi32Fromi8(env::environment_t id, env::addr_t address) {
	return jsMemoryReadi32Fromi8(id, address);
}
uint32_t env::bridge::Memory::Readi32Fromu16(env::environment_t id, env::addr_t address) {
	return jsMemoryReadi32Fromu16(id, address);
}
uint32_t env::bridge::Memory::Readi32Fromi16(env::environment_t id, env::addr_t address) {
	return jsMemoryReadi32Fromi16(id, address);
}
uint32_t env::bridge::Memory::Readi32(env::environment_t id, env::addr_t address) {
	return jsMemoryReadi32(id, address);
}
uint64_t env::bridge::Memory::Readi64(env::environment_t id, env::addr_t address) {
	return jsMemoryReadi64(id, address);
}
float env::bridge::Memory::Readf32(env::environment_t id, env::addr_t address) {
	return jsMemoryReadf32(id, address);
}
double env::bridge::Memory::Readf64(env::environment_t id, env::addr_t address) {
	return jsMemoryReadf64(id, address);
}

void env::bridge::Memory::Writei32Fromu8(env::environment_t id, env::addr_t address, uint32_t value) {
	jsMemoryWritei32Fromu8(id, address, value);
}
void env::bridge::Memory::Writei32Fromi8(env::environment_t id, env::addr_t address, uint32_t value) {
	jsMemoryWritei32Fromi8(id, address, value);
}
void env::bridge::Memory::Writei32Fromu16(env::environment_t id, env::addr_t address, uint32_t value) {
	jsMemoryWritei32Fromu16(id, address, value);
}
void env::bridge::Memory::Writei32Fromi16(env::environment_t id, env::addr_t address, uint32_t value) {
	jsMemoryWritei32Fromi16(id, address, value);
}
void env::bridge::Memory::Writei32(env::environment_t id, env::addr_t address, uint32_t value) {
	jsMemoryWritei32(id, address, value);
}
void env::bridge::Memory::Writei64(env::environment_t id, env::addr_t address, uint64_t value) {
	jsMemoryWritei64(id, address, value);
}
void env::bridge::Memory::Writef32(env::environment_t id, env::addr_t address, float value) {
	jsMemoryWritef32(id, address, value);
}
void env::bridge::Memory::Writef64(env::environment_t id, env::addr_t address, double value) {
	jsMemoryWritef64(id, address, value);
}

uint32_t env::bridge::Memory::Executei32Fromu8(env::environment_t id, env::addr_t address) {
	return jsMemoryExecutei32Fromu8(id, address);
}
uint32_t env::bridge::Memory::Executei32Fromi8(env::environment_t id, env::addr_t address) {
	return jsMemoryExecutei32Fromi8(id, address);
}
uint32_t env::bridge::Memory::Executei32Fromu16(env::environment_t id, env::addr_t address) {
	return jsMemoryExecutei32Fromu16(id, address);
}
uint32_t env::bridge::Memory::Executei32Fromi16(env::environment_t id, env::addr_t address) {
	return jsMemoryExecutei32Fromi16(id, address);
}
uint32_t env::bridge::Memory::Executei32(env::environment_t id, env::addr_t address) {
	return jsMemoryExecutei32(id, address);
}
uint64_t env::bridge::Memory::Executei64(env::environment_t id, env::addr_t address) {
	return jsMemoryExecutei64(id, address);
}
float env::bridge::Memory::Executef32(env::environment_t id, env::addr_t address) {
	return jsMemoryExecutef32(id, address);
}
double env::bridge::Memory::Executef64(env::environment_t id, env::addr_t address) {
	return jsMemoryExecutef64(id, address);
}
#else
#error Currently not supported
#endif
