#include "bridge-memory.h"

static env::bridge::Memory::LookupOutput LastResult = { 0 };

extern "C" uint64_t MemoryPerformLookup(uint64_t self, uint64_t address, uint32_t size, uint32_t usage) {
	LastResult = env::bridge::Memory::Lookup(self, address, size, usage);
	return LastResult.address;
}
extern "C" uint32_t MemoryLastLookupOffset() {
	return LastResult.offset;
}
extern "C" uint32_t MemoryLastLookupSize() {
	return LastResult.size;
}

env::bridge::Memory::LookupOutput env::bridge::Memory::Lookup(uint64_t self, uint64_t address, uint32_t size, uint32_t usage) {
	env::MemoryRegion reg = reinterpret_cast<env::Memory*>(self)->fLookup(address, size, usage);
	return { reg.address, reg.physical, reg.size };
}

#ifdef EMSCRIPTEN_COMPILATION

extern "C" {
	uint32_t jsMemoryReadi32Fromu8(uint32_t id, uint64_t address);
	uint32_t jsMemoryReadi32Fromi8(uint32_t id, uint64_t address);
	uint32_t jsMemoryReadi32Fromu16(uint32_t id, uint64_t address);
	uint32_t jsMemoryReadi32Fromi16(uint32_t id, uint64_t address);
	uint32_t jsMemoryReadi32(uint32_t id, uint64_t address);
	uint64_t jsMemoryReadi64(uint32_t id, uint64_t address);
	float jsMemoryReadf32(uint32_t id, uint64_t address);
	double jsMemoryReadf64(uint32_t id, uint64_t address);
	void jsMemoryWritei32Fromu8(uint32_t id, uint64_t address, uint32_t value);
	void jsMemoryWritei32Fromi8(uint32_t id, uint64_t address, uint32_t value);
	void jsMemoryWritei32Fromu16(uint32_t id, uint64_t address, uint32_t value);
	void jsMemoryWritei32Fromi16(uint32_t id, uint64_t address, uint32_t value);
	void jsMemoryWritei32(uint32_t id, uint64_t address, uint32_t value);
	void jsMemoryWritei64(uint32_t id, uint64_t address, uint64_t value);
	void jsMemoryWritef32(uint32_t id, uint64_t address, float value);
	void jsMemoryWritef64(uint32_t id, uint64_t address, double value);
	uint32_t jsMemoryExecutei32Fromu8(uint32_t id, uint64_t address);
	uint32_t jsMemoryExecutei32Fromi8(uint32_t id, uint64_t address);
	uint32_t jsMemoryExecutei32Fromu16(uint32_t id, uint64_t address);
	uint32_t jsMemoryExecutei32Fromi16(uint32_t id, uint64_t address);
	uint32_t jsMemoryExecutei32(uint32_t id, uint64_t address);
	uint64_t jsMemoryExecutei64(uint32_t id, uint64_t address);
	float jsMemoryExecutef32(uint32_t id, uint64_t address);
	double jsMemoryExecutef64(uint32_t id, uint64_t address);
}

uint32_t env::bridge::Memory::Readi32Fromu8(env::id_t id, env::addr_t address) {
	return jsMemoryReadi32Fromu8(id, address);
}
uint32_t env::bridge::Memory::Readi32Fromi8(env::id_t id, env::addr_t address) {
	return jsMemoryReadi32Fromi8(id, address);
}
uint32_t env::bridge::Memory::Readi32Fromu16(env::id_t id, env::addr_t address) {
	return jsMemoryReadi32Fromu16(id, address);
}
uint32_t env::bridge::Memory::Readi32Fromi16(env::id_t id, env::addr_t address) {
	return jsMemoryReadi32Fromi16(id, address);
}
uint32_t env::bridge::Memory::Readi32(env::id_t id, env::addr_t address) {
	return jsMemoryReadi32(id, address);
}
uint64_t env::bridge::Memory::Readi64(env::id_t id, env::addr_t address) {
	return jsMemoryReadi64(id, address);
}
float env::bridge::Memory::Readf32(env::id_t id, env::addr_t address) {
	return jsMemoryReadf32(id, address);
}
double env::bridge::Memory::Readf64(env::id_t id, env::addr_t address) {
	return jsMemoryReadf64(id, address);
}

void env::bridge::Memory::Writei32Fromu8(env::id_t id, env::addr_t address, uint32_t value) {
	jsMemoryWritei32Fromu8(id, address, value);
}
void env::bridge::Memory::Writei32Fromi8(env::id_t id, env::addr_t address, uint32_t value) {
	jsMemoryWritei32Fromi8(id, address, value);
}
void env::bridge::Memory::Writei32Fromu16(env::id_t id, env::addr_t address, uint32_t value) {
	jsMemoryWritei32Fromu16(id, address, value);
}
void env::bridge::Memory::Writei32Fromi16(env::id_t id, env::addr_t address, uint32_t value) {
	jsMemoryWritei32Fromi16(id, address, value);
}
void env::bridge::Memory::Writei32(env::id_t id, env::addr_t address, uint32_t value) {
	jsMemoryWritei32(id, address, value);
}
void env::bridge::Memory::Writei64(env::id_t id, env::addr_t address, uint64_t value) {
	jsMemoryWritei64(id, address, value);
}
void env::bridge::Memory::Writef32(env::id_t id, env::addr_t address, float value) {
	jsMemoryWritef32(id, address, value);
}
void env::bridge::Memory::Writef64(env::id_t id, env::addr_t address, double value) {
	jsMemoryWritef64(id, address, value);
}

uint32_t env::bridge::Memory::Executei32Fromu8(env::id_t id, env::addr_t address) {
	return jsMemoryExecutei32Fromu8(id, address);
}
uint32_t env::bridge::Memory::Executei32Fromi8(env::id_t id, env::addr_t address) {
	return jsMemoryExecutei32Fromi8(id, address);
}
uint32_t env::bridge::Memory::Executei32Fromu16(env::id_t id, env::addr_t address) {
	return jsMemoryExecutei32Fromu16(id, address);
}
uint32_t env::bridge::Memory::Executei32Fromi16(env::id_t id, env::addr_t address) {
	return jsMemoryExecutei32Fromi16(id, address);
}
uint32_t env::bridge::Memory::Executei32(env::id_t id, env::addr_t address) {
	return jsMemoryExecutei32(id, address);
}
uint64_t env::bridge::Memory::Executei64(env::id_t id, env::addr_t address) {
	return jsMemoryExecutei64(id, address);
}
float env::bridge::Memory::Executef32(env::id_t id, env::addr_t address) {
	return jsMemoryExecutef32(id, address);
}
double env::bridge::Memory::Executef64(env::id_t id, env::addr_t address) {
	return jsMemoryExecutef64(id, address);
}

#else

// #error Currently not supported

uint32_t env::bridge::Memory::Readi32Fromu8(env::id_t id, env::addr_t address) {
	return {};
}
uint32_t env::bridge::Memory::Readi32Fromi8(env::id_t id, env::addr_t address) {
	return {};
}
uint32_t env::bridge::Memory::Readi32Fromu16(env::id_t id, env::addr_t address) {
	return {};
}
uint32_t env::bridge::Memory::Readi32Fromi16(env::id_t id, env::addr_t address) {
	return {};
}
uint32_t env::bridge::Memory::Readi32(env::id_t id, env::addr_t address) {
	return {};
}
uint64_t env::bridge::Memory::Readi64(env::id_t id, env::addr_t address) {
	return {};
}
float env::bridge::Memory::Readf32(env::id_t id, env::addr_t address) {
	return {};
}
double env::bridge::Memory::Readf64(env::id_t id, env::addr_t address) {
	return {};
}

void env::bridge::Memory::Writei32Fromu8(env::id_t id, env::addr_t address, uint32_t value) {}
void env::bridge::Memory::Writei32Fromi8(env::id_t id, env::addr_t address, uint32_t value) {}
void env::bridge::Memory::Writei32Fromu16(env::id_t id, env::addr_t address, uint32_t value) {}
void env::bridge::Memory::Writei32Fromi16(env::id_t id, env::addr_t address, uint32_t value) {}
void env::bridge::Memory::Writei32(env::id_t id, env::addr_t address, uint32_t value) {}
void env::bridge::Memory::Writei64(env::id_t id, env::addr_t address, uint64_t value) {}
void env::bridge::Memory::Writef32(env::id_t id, env::addr_t address, float value) {}
void env::bridge::Memory::Writef64(env::id_t id, env::addr_t address, double value) {}

uint32_t env::bridge::Memory::Executei32Fromu8(env::id_t id, env::addr_t address) {
	return {};
}
uint32_t env::bridge::Memory::Executei32Fromi8(env::id_t id, env::addr_t address) {
	return {};
}
uint32_t env::bridge::Memory::Executei32Fromu16(env::id_t id, env::addr_t address) {
	return {};
}
uint32_t env::bridge::Memory::Executei32Fromi16(env::id_t id, env::addr_t address) {
	return {};
}
uint32_t env::bridge::Memory::Executei32(env::id_t id, env::addr_t address) {
	return {};
}
uint64_t env::bridge::Memory::Executei64(env::id_t id, env::addr_t address) {
	return {};
}
float env::bridge::Memory::Executef32(env::id_t id, env::addr_t address) {
	return {};
}
double env::bridge::Memory::Executef64(env::id_t id, env::addr_t address) {
	return {};
}

#endif
