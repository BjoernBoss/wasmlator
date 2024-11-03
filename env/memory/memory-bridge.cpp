#include "../env-process.h"
#include "../../interface/interface.h"

void env::bridge::Memory::Lookup(uint64_t process, uint64_t address, uint32_t size, uint32_t usage) {
	reinterpret_cast<env::Process*>(process)->memory().pMapper.lookup(address, size, usage);
}
uint64_t env::bridge::Memory::LookupAddress(uint64_t process) {
	return reinterpret_cast<env::Process*>(process)->memory().pMapper.lastLookup().address;
}
uint32_t env::bridge::Memory::LookupSize(uint64_t process) {
	return reinterpret_cast<env::Process*>(process)->memory().pMapper.lastLookup().size;
}
uint32_t env::bridge::Memory::LookupPhysical(uint64_t process) {
	return reinterpret_cast<env::Process*>(process)->memory().pMapper.lastLookup().physical;
}

bool env::bridge::Memory::ExpandPhysical(env::id_t id, uint32_t pages) {
	return (mem_expand_physical(id, pages) == 0);
}
void env::bridge::Memory::MovePhysical(env::id_t id, env::physical_t dest, env::physical_t source, uint32_t size) {
	mem_move_physical(id, dest, source, size);
}
void env::bridge::Memory::FlushCaches(env::id_t id) {
	mem_flush_caches(id);
}

uint32_t env::bridge::Memory::Readi32Fromu8(env::id_t id, env::guest_t address) {
	return mem_read_u8_i32(id, address);
}
uint32_t env::bridge::Memory::Readi32Fromi8(env::id_t id, env::guest_t address) {
	return mem_read_i8_i32(id, address);
}
uint32_t env::bridge::Memory::Readi32Fromu16(env::id_t id, env::guest_t address) {
	return mem_read_u16_i32(id, address);
}
uint32_t env::bridge::Memory::Readi32Fromi16(env::id_t id, env::guest_t address) {
	return mem_read_i16_i32(id, address);
}
uint32_t env::bridge::Memory::Readi32(env::id_t id, env::guest_t address) {
	return mem_read_i32(id, address);
}
uint64_t env::bridge::Memory::Readi64(env::id_t id, env::guest_t address) {
	return mem_read_i64(id, address);
}
float env::bridge::Memory::Readf32(env::id_t id, env::guest_t address) {
	return mem_read_f32(id, address);
}
double env::bridge::Memory::Readf64(env::id_t id, env::guest_t address) {
	return mem_read_f64(id, address);
}

void env::bridge::Memory::Writei32Fromu8(env::id_t id, env::guest_t address, uint32_t value) {
	mem_write_u8_i32(id, address, value);
}
void env::bridge::Memory::Writei32Fromi8(env::id_t id, env::guest_t address, uint32_t value) {
	mem_write_i8_i32(id, address, value);
}
void env::bridge::Memory::Writei32Fromu16(env::id_t id, env::guest_t address, uint32_t value) {
	mem_write_u16_i32(id, address, value);
}
void env::bridge::Memory::Writei32Fromi16(env::id_t id, env::guest_t address, uint32_t value) {
	mem_write_i16_i32(id, address, value);
}
void env::bridge::Memory::Writei32(env::id_t id, env::guest_t address, uint32_t value) {
	mem_write_i32(id, address, value);
}
void env::bridge::Memory::Writei64(env::id_t id, env::guest_t address, uint64_t value) {
	mem_write_i64(id, address, value);
}
void env::bridge::Memory::Writef32(env::id_t id, env::guest_t address, float value) {
	mem_write_f32(id, address, value);
}
void env::bridge::Memory::Writef64(env::id_t id, env::guest_t address, double value) {
	mem_write_f64(id, address, value);
}

uint32_t env::bridge::Memory::Codei32Fromu8(env::id_t id, env::guest_t address) {
	return mem_code_u8_i32(id, address);
}
uint32_t env::bridge::Memory::Codei32Fromi8(env::id_t id, env::guest_t address) {
	return mem_code_i8_i32(id, address);
}
uint32_t env::bridge::Memory::Codei32Fromu16(env::id_t id, env::guest_t address) {
	return mem_code_u16_i32(id, address);
}
uint32_t env::bridge::Memory::Codei32Fromi16(env::id_t id, env::guest_t address) {
	return mem_code_i16_i32(id, address);
}
uint32_t env::bridge::Memory::Codei32(env::id_t id, env::guest_t address) {
	return mem_code_i32(id, address);
}
uint64_t env::bridge::Memory::Codei64(env::id_t id, env::guest_t address) {
	return mem_code_i64(id, address);
}
float env::bridge::Memory::Codef32(env::id_t id, env::guest_t address) {
	return mem_code_f32(id, address);
}
double env::bridge::Memory::Codef64(env::id_t id, env::guest_t address) {
	return mem_code_f64(id, address);
}
