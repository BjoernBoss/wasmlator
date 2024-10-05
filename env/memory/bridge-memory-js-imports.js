function jsMemoryExpandPhysical(id, size) {
	return _env.core[id].mem_expand_physical(size);
}
function jsMemoryMovePhysical(id, dest, source, size) {
	return _env.core[id].mem_move_physical(dest, source, size);
}
function jsFlushCaches(id) {
	return _env.core[id].mem_flush_caches();
}
	
function jsMemoryReadi32Fromu8(id, address) {
	return _env.core[id].mem_read_u8_i32(address);
}
function jsMemoryReadi32Fromi8(id, address) {
	return _env.core[id].mem_read_i8_i32(address);
}
function jsMemoryReadi32Fromu16(id, address) {
	return _env.core[id].mem_read_u16_i32(address);
}
function jsMemoryReadi32Fromi16(id, address) {
	return _env.core[id].mem_read_i16_i32(address);
}
function jsMemoryReadi32(id, address) {
	return _env.core[id].mem_read_i32(address);
}
function jsMemoryReadi64(id, address) {
	return _env.core[id].mem_read_i64(address);
}
function jsMemoryReadf32(id, address) {
	return _env.core[id].mem_read_f32(address);
}
function jsMemoryReadf64(id, address) {
	return _env.core[id].mem_read_f64(address);
}

function jsMemoryWritei32Fromu8(id, address, value) {
	_env.core[id].mem_write_u8_i32(address, value);
}
function jsMemoryWritei32Fromi8(id, address, value) {
	_env.core[id].mem_write_i8_i32(address, value);
}
function jsMemoryWritei32Fromu16(id, address, value) {
	_env.core[id].mem_write_u16_i32(address, value);
}
function jsMemoryWritei32Fromi16(id, address, value) {
	_env.core[id].mem_write_i16_i32(address, value);
}
function jsMemoryWritei32(id, address, value) {
	_env.core[id].mem_write_i32(address, value);
}
function jsMemoryWritei64(id, address, value) {
	_env.core[id].mem_write_i64(address, value);
}
function jsMemoryWritef32(id, address, value) {
	_env.core[id].mem_write_f32(address, value);
}
function jsMemoryWritef64(id, address, value) {
	_env.core[id].mem_write_f64(address, value);
}

function jsMemoryExecutei32Fromu8(id, address) {
	return _env.core[id].mem_execute_u8_i32(address);
}
function jsMemoryExecutei32Fromi8(id, address) {
	return _env.core[id].mem_execute_i8_i32(address);
}
function jsMemoryExecutei32Fromu16(id, address) {
	return _env.core[id].mem_execute_u16_i32(address);
}
function jsMemoryExecutei32Fromi16(id, address) {
	return _env.core[id].mem_execute_i16_i32(address);
}
function jsMemoryExecutei32(id, address) {
	return _env.core[id].mem_execute_i32(address);
}
function jsMemoryExecutei64(id, address) {
	return _env.core[id].mem_execute_i64(address);
}
function jsMemoryExecutef32(id, address) {
	return _env.core[id].mem_execute_f32(address);
}
function jsMemoryExecutef64(id, address) {
	return _env.core[id].mem_execute_f64(address);
}


mergeInto(LibraryManager.library, {
	jsMemoryExpandPhysical,
	jsMemoryMovePhysical,
	jsFlushCaches,
	jsMemoryReadi32Fromu8,
	jsMemoryReadi32Fromi8,
	jsMemoryReadi32Fromu16,
	jsMemoryReadi32Fromi16,
	jsMemoryReadi32,
	jsMemoryReadi64,
	jsMemoryReadf32,
	jsMemoryReadf64,
	jsMemoryWritei32Fromu8,
	jsMemoryWritei32Fromi8,
	jsMemoryWritei32Fromu16,
	jsMemoryWritei32Fromi16,
	jsMemoryWritei32,
	jsMemoryWritei64,
	jsMemoryWritef32,
	jsMemoryWritef64,
	jsMemoryExecutei32Fromu8,
	jsMemoryExecutei32Fromi8,
	jsMemoryExecutei32Fromu16,
	jsMemoryExecutei32Fromi16,
	jsMemoryExecutei32,
	jsMemoryExecutei64,
	jsMemoryExecutef32,
	jsMemoryExecutef64
});
