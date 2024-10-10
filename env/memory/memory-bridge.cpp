#include "memory-bridge.h"
#include "../../interface/interface.h"

bool env::bridge::Memory::MMap(uint64_t self, uint64_t address, uint32_t size, uint32_t usage) {
	return reinterpret_cast<env::Memory*>(self)->pMapper.mmap(address, size, usage);
}
void env::bridge::Memory::MUnmap(uint64_t self, uint64_t address, uint32_t size) {
	reinterpret_cast<env::Memory*>(self)->pMapper.munmap(address, size);
}
void env::bridge::Memory::MProtect(uint64_t self, uint64_t address, uint32_t size, uint32_t usage) {
	reinterpret_cast<env::Memory*>(self)->pMapper.mprotect(address, size, usage);
}

void env::bridge::Memory::Lookup(uint64_t self, uint64_t address, uint32_t size, uint32_t usage) {
	reinterpret_cast<env::Memory*>(self)->pMapper.lookup(address, size, usage);
}
uint64_t env::bridge::Memory::LookupAddress(uint64_t self) {
	return reinterpret_cast<env::Memory*>(self)->pMapper.lastLookup().address;
}
uint32_t env::bridge::Memory::LookupSize(uint64_t self) {
	return reinterpret_cast<env::Memory*>(self)->pMapper.lastLookup().size;
}
uint32_t env::bridge::Memory::LookupPhysical(uint64_t self) {
	return reinterpret_cast<env::Memory*>(self)->pMapper.lastLookup().physical;
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

uint32_t env::bridge::Memory::Readi32Fromu8(env::id_t id, env::addr_t address) {
	return mem_read_u8_i32(id, address);
}
uint32_t env::bridge::Memory::Readi32Fromi8(env::id_t id, env::addr_t address) {
	return mem_read_i8_i32(id, address);
}
uint32_t env::bridge::Memory::Readi32Fromu16(env::id_t id, env::addr_t address) {
	return mem_read_u16_i32(id, address);
}
uint32_t env::bridge::Memory::Readi32Fromi16(env::id_t id, env::addr_t address) {
	return mem_read_i16_i32(id, address);
}
uint32_t env::bridge::Memory::Readi32(env::id_t id, env::addr_t address) {
	return mem_read_i32(id, address);
}
uint64_t env::bridge::Memory::Readi64(env::id_t id, env::addr_t address) {
	return mem_read_i64(id, address);
}
float env::bridge::Memory::Readf32(env::id_t id, env::addr_t address) {
	return mem_read_f32(id, address);
}
double env::bridge::Memory::Readf64(env::id_t id, env::addr_t address) {
	return mem_read_f64(id, address);
}

void env::bridge::Memory::Writei32Fromu8(env::id_t id, env::addr_t address, uint32_t value) {
	mem_write_u8_i32(id, address, value);
}
void env::bridge::Memory::Writei32Fromi8(env::id_t id, env::addr_t address, uint32_t value) {
	mem_write_i8_i32(id, address, value);
}
void env::bridge::Memory::Writei32Fromu16(env::id_t id, env::addr_t address, uint32_t value) {
	mem_write_u16_i32(id, address, value);
}
void env::bridge::Memory::Writei32Fromi16(env::id_t id, env::addr_t address, uint32_t value) {
	mem_write_i16_i32(id, address, value);
}
void env::bridge::Memory::Writei32(env::id_t id, env::addr_t address, uint32_t value) {
	mem_write_i32(id, address, value);
}
void env::bridge::Memory::Writei64(env::id_t id, env::addr_t address, uint64_t value) {
	mem_write_i64(id, address, value);
}
void env::bridge::Memory::Writef32(env::id_t id, env::addr_t address, float value) {
	mem_write_f32(id, address, value);
}
void env::bridge::Memory::Writef64(env::id_t id, env::addr_t address, double value) {
	mem_write_f64(id, address, value);
}

uint32_t env::bridge::Memory::Executei32Fromu8(env::id_t id, env::addr_t address) {
	return mem_execute_u8_i32(id, address);
}
uint32_t env::bridge::Memory::Executei32Fromi8(env::id_t id, env::addr_t address) {
	return mem_execute_i8_i32(id, address);
}
uint32_t env::bridge::Memory::Executei32Fromu16(env::id_t id, env::addr_t address) {
	return mem_execute_u16_i32(id, address);
}
uint32_t env::bridge::Memory::Executei32Fromi16(env::id_t id, env::addr_t address) {
	return mem_execute_i16_i32(id, address);
}
uint32_t env::bridge::Memory::Executei32(env::id_t id, env::addr_t address) {
	return mem_execute_i32(id, address);
}
uint64_t env::bridge::Memory::Executei64(env::id_t id, env::addr_t address) {
	return mem_execute_i64(id, address);
}
float env::bridge::Memory::Executef32(env::id_t id, env::addr_t address) {
	return mem_execute_f32(id, address);
}
double env::bridge::Memory::Executef64(env::id_t id, env::addr_t address) {
	return mem_execute_f64(id, address);
}
