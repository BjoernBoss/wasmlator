#include "../env-process.h"
#include "../../interface/interface.h"

void env::detail::MemoryBridge::Lookup(uint64_t address, uint32_t size, uint32_t usage) {
	env::Instance()->memory().pMapper.lookup(address, size, usage);
}
uint64_t env::detail::MemoryBridge::LookupAddress() {
	return env::Instance()->memory().pMapper.lastLookup().address;
}
uint32_t env::detail::MemoryBridge::LookupSize() {
	return env::Instance()->memory().pMapper.lastLookup().size;
}
uint32_t env::detail::MemoryBridge::LookupPhysical() {
	return env::Instance()->memory().pMapper.lastLookup().physical;
}

bool env::detail::MemoryBridge::ExpandPhysical(uint32_t pages) {
	return (mem_expand_physical(pages) == 0);
}
void env::detail::MemoryBridge::MovePhysical(env::physical_t dest, env::physical_t source, uint32_t size) {
	mem_move_physical(dest, source, size);
}
void env::detail::MemoryBridge::FlushCaches() {
	mem_flush_caches();
}

uint32_t env::detail::MemoryBridge::Readi32Fromu8(env::guest_t address) {
	return mem_read_u8_i32(address);
}
uint32_t env::detail::MemoryBridge::Readi32Fromi8(env::guest_t address) {
	return mem_read_i8_i32(address);
}
uint32_t env::detail::MemoryBridge::Readi32Fromu16(env::guest_t address) {
	return mem_read_u16_i32(address);
}
uint32_t env::detail::MemoryBridge::Readi32Fromi16(env::guest_t address) {
	return mem_read_i16_i32(address);
}
uint32_t env::detail::MemoryBridge::Readi32(env::guest_t address) {
	return mem_read_i32(address);
}
uint64_t env::detail::MemoryBridge::Readi64(env::guest_t address) {
	return mem_read_i64(address);
}
float env::detail::MemoryBridge::Readf32(env::guest_t address) {
	return mem_read_f32(address);
}
double env::detail::MemoryBridge::Readf64(env::guest_t address) {
	return mem_read_f64(address);
}

void env::detail::MemoryBridge::Writei32Fromu8(env::guest_t address, uint32_t value) {
	mem_write_u8_i32(address, value);
}
void env::detail::MemoryBridge::Writei32Fromi8(env::guest_t address, uint32_t value) {
	mem_write_i8_i32(address, value);
}
void env::detail::MemoryBridge::Writei32Fromu16(env::guest_t address, uint32_t value) {
	mem_write_u16_i32(address, value);
}
void env::detail::MemoryBridge::Writei32Fromi16(env::guest_t address, uint32_t value) {
	mem_write_i16_i32(address, value);
}
void env::detail::MemoryBridge::Writei32(env::guest_t address, uint32_t value) {
	mem_write_i32(address, value);
}
void env::detail::MemoryBridge::Writei64(env::guest_t address, uint64_t value) {
	mem_write_i64(address, value);
}
void env::detail::MemoryBridge::Writef32(env::guest_t address, float value) {
	mem_write_f32(address, value);
}
void env::detail::MemoryBridge::Writef64(env::guest_t address, double value) {
	mem_write_f64(address, value);
}

uint32_t env::detail::MemoryBridge::Codei32Fromu8(env::guest_t address) {
	return mem_code_u8_i32(address);
}
uint32_t env::detail::MemoryBridge::Codei32Fromi8(env::guest_t address) {
	return mem_code_i8_i32(address);
}
uint32_t env::detail::MemoryBridge::Codei32Fromu16(env::guest_t address) {
	return mem_code_u16_i32(address);
}
uint32_t env::detail::MemoryBridge::Codei32Fromi16(env::guest_t address) {
	return mem_code_i16_i32(address);
}
uint32_t env::detail::MemoryBridge::Codei32(env::guest_t address) {
	return mem_code_i32(address);
}
uint64_t env::detail::MemoryBridge::Codei64(env::guest_t address) {
	return mem_code_i64(address);
}
float env::detail::MemoryBridge::Codef32(env::guest_t address) {
	return mem_code_f32(address);
}
double env::detail::MemoryBridge::Codef64(env::guest_t address) {
	return mem_code_f64(address);
}
