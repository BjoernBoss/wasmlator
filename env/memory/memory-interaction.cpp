#include "../env-process.h"

namespace I = wasm::inst;

env::detail::MemoryInteraction::MemoryInteraction(env::Process* process) : pProcess{ process } {}

void env::detail::MemoryInteraction::fCheckCache(uint32_t cache) const {
	if (cache >= pCacheCount)
		pProcess->fail(u8"Cache [", cache, u8"] out of bounds as only [", pCacheCount, u8"] caches have been defined");
}
void env::detail::MemoryInteraction::fMakeAddress(wasm::Sink& sink, const env::ModuleState& state, uint32_t cache, const wasm::Variable& i64Address, const wasm::Function& lookup, env::MemoryType type) const {
	/* compute the offset into the current cached region */
	sink[I::Local::Get(i64Address)];
	sink[I::U32::Const(pCacheAddress + cache * sizeof(MemoryInteraction::MemCache))];
	sink[I::U64::Load(state.management, offsetof(MemoryInteraction::MemCache, address))];
	sink[I::U64::Sub()];
	sink[I::Local::Tee(i64Address)];

	/* check if the address lies in the range */
	sink[I::U32::Const(pCacheAddress + cache * sizeof(MemoryInteraction::MemCache))];
	switch (type) {
	case env::MemoryType::u8To32:
	case env::MemoryType::u8To64:
	case env::MemoryType::i8To32:
	case env::MemoryType::i8To64:
		sink[I::U64::Load32(state.management, offsetof(MemoryInteraction::MemCache, size1))];
		break;
	case env::MemoryType::u16To32:
	case env::MemoryType::u16To64:
	case env::MemoryType::i16To32:
	case env::MemoryType::i16To64:
		sink[I::U64::Load32(state.management, offsetof(MemoryInteraction::MemCache, size2))];
		break;
	case env::MemoryType::i32:
	case env::MemoryType::f32:
	case env::MemoryType::u32To64:
	case env::MemoryType::i32To64:
		sink[I::U64::Load32(state.management, offsetof(MemoryInteraction::MemCache, size4))];
		break;
	case env::MemoryType::i64:
	case env::MemoryType::f64:
		sink[I::U64::Load32(state.management, offsetof(MemoryInteraction::MemCache, size8))];
		break;
	}
	sink[I::U64::GreaterEqual()];

	{
		/* greater-equal, perform mem-cache lookup */
		wasm::IfThen _if{ sink, {}, {}, { wasm::Type::i32 } };

		/* recover the original address */
		sink[I::Local::Get(i64Address)];
		sink[I::U32::Const(pCacheAddress + cache * sizeof(MemoryInteraction::MemCache))];
		sink[I::U64::Load(state.management, offsetof(MemoryInteraction::MemCache, address))];
		sink[I::U64::Add()];

		/* write the current size to the stack */
		switch (type) {
		case env::MemoryType::u8To32:
		case env::MemoryType::u8To64:
		case env::MemoryType::i8To32:
		case env::MemoryType::i8To64:
			sink[I::U32::Const(1)];
			break;
		case env::MemoryType::u16To32:
		case env::MemoryType::u16To64:
		case env::MemoryType::i16To32:
		case env::MemoryType::i16To64:
			sink[I::U32::Const(2)];
			break;
		case env::MemoryType::i32:
		case env::MemoryType::f32:
		case env::MemoryType::u32To64:
		case env::MemoryType::i32To64:
			sink[I::U32::Const(4)];
			break;
		case env::MemoryType::i64:
		case env::MemoryType::f64:
			sink[I::U32::Const(8)];
			break;
		}

		/* perform the call to patch the cache (leaves the new absolute address as i32 on the stack) */
		sink[I::U32::Const(pCacheAddress + cache * sizeof(MemoryInteraction::MemCache))];
		sink[I::Call::Direct(lookup)];

		/* less-equal, compute the final absolute address */
		_if.otherwise();
		sink[I::Local::Get(i64Address)];
		sink[I::U64::Shrink()];
		sink[I::U32::Const(pCacheAddress + cache * sizeof(MemoryInteraction::MemCache))];
		sink[I::U32::Load(state.management, offsetof(MemoryInteraction::MemCache, physical))];
		sink[I::U32::Add()];
	}
}
void env::detail::MemoryInteraction::fMakeLookup(const env::CoreState& state, const wasm::Function& function, uint32_t usage) const {
	wasm::Sink sink{ function };
	wasm::Variable address = sink.parameter(0), size = sink.parameter(1), cacheAddress = sink.parameter(2);

	/* allocate the local variables necessary to write the temporary result back */
	wasm::Variable outAddr = sink.local(wasm::Type::i64, u8"out_address");
	wasm::Variable outPhys = sink.local(wasm::Type::i32, u8"out_physical");
	wasm::Variable outSize = sink.local(wasm::Type::i32, u8"out_size");

	/* perform the call */
	sink[I::U64::Const(pProcess)];
	sink[I::Local::Get(address)];
	sink[I::Local::Get(size)];
	sink[I::U32::Const(usage)];
	sink[I::Call::Direct(state.mem.lookup)];

	/* fetch the result */
	sink[I::U64::Const(pProcess)];
	sink[I::Call::Direct(state.mem.getAddress)];
	sink[I::Local::Set(outAddr)];
	sink[I::U64::Const(pProcess)];
	sink[I::Call::Direct(state.mem.getPhysical)];
	sink[I::Local::Set(outPhys)];
	sink[I::U64::Const(pProcess)];
	sink[I::Call::Direct(state.mem.getSize)];
	sink[I::Local::Set(outSize)];

	/* write the address and physical offset back */
	sink[I::Local::Get(cacheAddress)];
	sink[I::Local::Get(outAddr)];
	sink[I::U64::Store(state.module.management, offsetof(MemoryInteraction::MemCache, address))];
	sink[I::Local::Get(cacheAddress)];
	sink[I::Local::Get(outPhys)];
	sink[I::U32::Store(state.module.management, offsetof(MemoryInteraction::MemCache, physical))];

	/* write the different sizes back */
	for (uint32_t i = 1; i <= 8; i *= 2) {
		sink[I::Local::Get(cacheAddress)];
		sink[I::Local::Get(outSize)];
		sink[I::U32::Const(i)];
		sink[I::U32::Sub()];
		switch (i) {
		case 1:
			sink[I::U32::Store(state.module.management, offsetof(MemoryInteraction::MemCache, size1))];
			break;
		case 2:
			sink[I::U32::Store(state.module.management, offsetof(MemoryInteraction::MemCache, size2))];
			break;
		case 4:
			sink[I::U32::Store(state.module.management, offsetof(MemoryInteraction::MemCache, size4))];
			break;
		case 8:
			sink[I::U32::Store(state.module.management, offsetof(MemoryInteraction::MemCache, size8))];
			break;
		}
	}

	/* compute the final actual offset */
	sink[I::Local::Get(address)];
	sink[I::Local::Get(outAddr)];
	sink[I::U64::Sub()];
	sink[I::U64::Shrink()];
	sink[I::Local::Get(outPhys)];
	sink[I::U32::Add()];
}
void env::detail::MemoryInteraction::fMakeRead(wasm::Sink& sink, const wasm::Variable& i64Address, const env::ModuleState& state, uint32_t cache, env::MemoryType type) const {
	fMakeAddress(sink, state, cache, i64Address, state.mem.read, type);

	/* add the final read-instruction */
	switch (type) {
	case env::MemoryType::u8To32:
		sink[I::U32::Load8(state.physical)];
		break;
	case env::MemoryType::u16To32:
		sink[I::U32::Load16(state.physical)];
		break;
	case env::MemoryType::u8To64:
		sink[I::U64::Load8(state.physical)];
		break;
	case env::MemoryType::u16To64:
		sink[I::U64::Load16(state.physical)];
		break;
	case env::MemoryType::u32To64:
		sink[I::U64::Load32(state.physical)];
		break;
	case env::MemoryType::i8To32:
		sink[I::I32::Load8(state.physical)];
		break;
	case env::MemoryType::i16To32:
		sink[I::I32::Load16(state.physical)];
		break;
	case env::MemoryType::i8To64:
		sink[I::I64::Load8(state.physical)];
		break;
	case env::MemoryType::i16To64:
		sink[I::I64::Load16(state.physical)];
		break;
	case env::MemoryType::i32To64:
		sink[I::I64::Load32(state.physical)];
		break;
	case env::MemoryType::i32:
		sink[I::U32::Load(state.physical)];
		break;
	case env::MemoryType::i64:
		sink[I::U64::Load(state.physical)];
		break;
	case env::MemoryType::f32:
		sink[I::F32::Load(state.physical)];
		break;
	case env::MemoryType::f64:
		sink[I::F64::Load(state.physical)];
		break;
	}
}
void env::detail::MemoryInteraction::fMakeWrite(wasm::Sink& sink, const wasm::Variable& i64Address, const wasm::Variable& value, const env::ModuleState& state, uint32_t cache, env::MemoryType type) const {
	fMakeAddress(sink, state, cache, i64Address, state.mem.write, type);

	/* write the value to the stack */
	sink[I::Local::Get(value)];

	/* add the final store-instruction */
	switch (type) {
	case env::MemoryType::u8To32:
	case env::MemoryType::i8To32:
		sink[I::U32::Store8(state.physical)];
		break;
	case env::MemoryType::u16To32:
	case env::MemoryType::i16To32:
		sink[I::U32::Store16(state.physical)];
		break;
	case env::MemoryType::u8To64:
	case env::MemoryType::i8To64:
		sink[I::U64::Store8(state.physical)];
		break;
	case env::MemoryType::u16To64:
	case env::MemoryType::i16To64:
		sink[I::U64::Store16(state.physical)];
		break;
	case env::MemoryType::u32To64:
	case env::MemoryType::i32To64:
		sink[I::U64::Store32(state.physical)];
		break;
	case env::MemoryType::i32:
		sink[I::U32::Store(state.physical)];
		break;
	case env::MemoryType::i64:
		sink[I::U64::Store(state.physical)];
		break;
	case env::MemoryType::f32:
		sink[I::F32::Store(state.physical)];
		break;
	case env::MemoryType::f64:
		sink[I::F64::Store(state.physical)];
		break;
	}
}
void env::detail::MemoryInteraction::fMakeExecute(wasm::Sink& sink, const wasm::Variable& i64Address, const env::ModuleState& state, uint32_t cache, env::MemoryType type) const {
	fMakeAddress(sink, state, cache, i64Address, state.mem.execute, type);

	/* add the final read-instruction */
	switch (type) {
	case env::MemoryType::u8To32:
		sink[I::U32::Load8(state.physical)];
		break;
	case env::MemoryType::u16To32:
		sink[I::U32::Load16(state.physical)];
		break;
	case env::MemoryType::u8To64:
		sink[I::U64::Load8(state.physical)];
		break;
	case env::MemoryType::u16To64:
		sink[I::U64::Load16(state.physical)];
		break;
	case env::MemoryType::u32To64:
		sink[I::U64::Load32(state.physical)];
		break;
	case env::MemoryType::i8To32:
		sink[I::I32::Load8(state.physical)];
		break;
	case env::MemoryType::i16To32:
		sink[I::I32::Load16(state.physical)];
		break;
	case env::MemoryType::i8To64:
		sink[I::I64::Load8(state.physical)];
		break;
	case env::MemoryType::i16To64:
		sink[I::I64::Load16(state.physical)];
		break;
	case env::MemoryType::i32To64:
		sink[I::I64::Load32(state.physical)];
		break;
	case env::MemoryType::i32:
		sink[I::U32::Load(state.physical)];
		break;
	case env::MemoryType::i64:
		sink[I::U64::Load(state.physical)];
		break;
	case env::MemoryType::f32:
		sink[I::F32::Load(state.physical)];
		break;
	case env::MemoryType::f64:
		sink[I::F64::Load(state.physical)];
		break;
	}
}
void env::detail::MemoryInteraction::fMakeAccess(wasm::Module& mod, const env::CoreState& state, wasm::Type type, std::u8string_view name, env::MemoryType memoryType) const {
	wasm::Sink read{ mod.function(str::Build<std::u8string>(u8"mem_read_", name), { wasm::Type::i64 }, { type }, wasm::Export{}) };
	wasm::Sink write{ mod.function(str::Build<std::u8string>(u8"mem_write_", name), { wasm::Type::i64, type }, {}, wasm::Export{}) };
	wasm::Sink execute{ mod.function(str::Build<std::u8string>(u8"mem_execute_", name), { wasm::Type::i64 }, { type }, wasm::Export{}) };

	/* make the read, write, and execute function */
	fMakeRead(read, read.parameter(0), state.module, pReadCache, memoryType);
	fMakeExecute(execute, execute.parameter(0), state.module, pExecuteCache, memoryType);
	fMakeWrite(write, write.parameter(0), write.parameter(1), state.module, pExecuteCache, memoryType);
}

uint32_t env::detail::MemoryInteraction::fReadi32Fromi8(env::guest_t address) const {
	return bridge::Memory::Readi32Fromi8(pProcess->context().id(), address);
}
uint32_t env::detail::MemoryInteraction::fReadi32Fromu8(env::guest_t address) const {
	return bridge::Memory::Readi32Fromu8(pProcess->context().id(), address);
}
uint32_t env::detail::MemoryInteraction::fReadi32Fromi16(env::guest_t address) const {
	return bridge::Memory::Readi32Fromi16(pProcess->context().id(), address);
}
uint32_t env::detail::MemoryInteraction::fReadi32Fromu16(env::guest_t address) const {
	return bridge::Memory::Readi32Fromu16(pProcess->context().id(), address);
}
uint32_t env::detail::MemoryInteraction::fReadi32(env::guest_t address) const {
	return bridge::Memory::Readi32(pProcess->context().id(), address);
}
uint64_t env::detail::MemoryInteraction::fReadi64(env::guest_t address) const {
	return bridge::Memory::Readi64(pProcess->context().id(), address);
}
float env::detail::MemoryInteraction::fReadf32(env::guest_t address) const {
	return bridge::Memory::Readf32(pProcess->context().id(), address);
}
double env::detail::MemoryInteraction::fReadf64(env::guest_t address) const {
	return bridge::Memory::Readf64(pProcess->context().id(), address);
}
void env::detail::MemoryInteraction::fWritei32Fromi8(env::guest_t address, uint32_t value) const {
	bridge::Memory::Writei32Fromi8(pProcess->context().id(), address, value);
}
void env::detail::MemoryInteraction::fWritei32Fromu8(env::guest_t address, uint32_t value) const {
	bridge::Memory::Writei32Fromu8(pProcess->context().id(), address, value);
}
void env::detail::MemoryInteraction::fWritei32Fromi16(env::guest_t address, uint32_t value) const {
	bridge::Memory::Writei32Fromi16(pProcess->context().id(), address, value);
}
void env::detail::MemoryInteraction::fWritei32Fromu16(env::guest_t address, uint32_t value) const {
	bridge::Memory::Writei32Fromu16(pProcess->context().id(), address, value);
}
void env::detail::MemoryInteraction::fWritei32(env::guest_t address, uint32_t value) const {
	bridge::Memory::Writei32(pProcess->context().id(), address, value);
}
void env::detail::MemoryInteraction::fWritei64(env::guest_t address, uint64_t value) const {
	bridge::Memory::Writei64(pProcess->context().id(), address, value);
}
void env::detail::MemoryInteraction::fWritef32(env::guest_t address, float value) const {
	bridge::Memory::Writef32(pProcess->context().id(), address, value);
}
void env::detail::MemoryInteraction::fWritef64(env::guest_t address, double value) const {
	bridge::Memory::Writef64(pProcess->context().id(), address, value);
}
uint32_t env::detail::MemoryInteraction::fExecutei32Fromi8(env::guest_t address) const {
	return bridge::Memory::Executei32Fromi8(pProcess->context().id(), address);
}
uint32_t env::detail::MemoryInteraction::fExecutei32Fromu8(env::guest_t address) const {
	return bridge::Memory::Executei32Fromu8(pProcess->context().id(), address);
}
uint32_t env::detail::MemoryInteraction::fExecutei32Fromi16(env::guest_t address) const {
	return bridge::Memory::Executei32Fromi16(pProcess->context().id(), address);
}
uint32_t env::detail::MemoryInteraction::fExecutei32Fromu16(env::guest_t address) const {
	return bridge::Memory::Executei32Fromu16(pProcess->context().id(), address);
}
uint32_t env::detail::MemoryInteraction::fExecutei32(env::guest_t address) const {
	return bridge::Memory::Executei32(pProcess->context().id(), address);
}
uint64_t env::detail::MemoryInteraction::fExecutei64(env::guest_t address) const {
	return bridge::Memory::Executei64(pProcess->context().id(), address);
}
float env::detail::MemoryInteraction::fExecutef32(env::guest_t address) const {
	return bridge::Memory::Executef32(pProcess->context().id(), address);
}
double env::detail::MemoryInteraction::fExecutef64(env::guest_t address) const {
	return bridge::Memory::Executef64(pProcess->context().id(), address);
}

uint32_t env::detail::MemoryInteraction::configureAndAllocate(uint32_t address, uint32_t caches) {
	/* setup the cache-indices for both the guest-application and the internal read/write/execute caches */
	pCacheCount = caches;
	pReadCache = pCacheCount + 0;
	pWriteCache = pCacheCount + 1;
	pExecuteCache = pCacheCount + 2;

	/* allocate the cache-entries from the management memory */
	pCacheAddress = address;
	return (pCacheCount + MemoryInteraction::InternalCaches) * uint32_t(sizeof(MemoryInteraction::MemCache));
}
void env::detail::MemoryInteraction::setupCoreImports(wasm::Module& mod, env::CoreState& state) const {
	/* add the import to the lookup-function */
	wasm::Prototype prototype = mod.prototype(u8"mem_lookup_type",
		{ { u8"process", wasm::Type::i64 }, { u8"addr", wasm::Type::i64 }, { u8"size", wasm::Type::i32 }, { u8"usage", wasm::Type::i32 } },
		{}
	);
	state.mem.lookup = mod.function(u8"lookup", prototype, wasm::Import{ u8"mem" });

	prototype = mod.prototype(u8"mem_lookup_i64_type", { { u8"process", wasm::Type::i64 } }, { wasm::Type::i64 });
	state.mem.getAddress = mod.function(u8"result_address", prototype, wasm::Import{ u8"mem" });

	prototype = mod.prototype(u8"mem_lookup_i32_type", { { u8"process", wasm::Type::i64 } }, { wasm::Type::i32 });
	state.mem.getPhysical = mod.function(u8"result_physical", prototype, wasm::Import{ u8"mem" });
	state.mem.getSize = mod.function(u8"result_size", prototype, wasm::Import{ u8"mem" });
}
void env::detail::MemoryInteraction::setupCoreBody(wasm::Module& mod, env::CoreState& state) const {
	/* add the functions for the page-patching (receive the address as parameter and return the new absolute address) */
	wasm::Prototype prototype = mod.prototype(u8"mem_lookup_usage_type", { { u8"addr", wasm::Type::i64 }, { u8"size", wasm::Type::i32 }, { u8"cache_address", wasm::Type::i32 } }, { wasm::Type::i32 });
	state.module.mem.read = mod.function(u8"mem_lookup_read", prototype, wasm::Export{});
	state.module.mem.write = mod.function(u8"mem_lookup_write", prototype, wasm::Export{});
	state.module.mem.execute = mod.function(u8"mem_lookup_execute", prototype, wasm::Export{});

	/* add the actual implementations */
	fMakeLookup(state, state.module.mem.read, env::MemoryUsage::Read);
	fMakeLookup(state, state.module.mem.write, env::MemoryUsage::Write);
	fMakeLookup(state, state.module.mem.execute, env::MemoryUsage::Execute);

	/* add the separate access functions */
	fMakeAccess(mod, state, wasm::Type::i32, u8"u8_i32", env::MemoryType::u8To32);
	fMakeAccess(mod, state, wasm::Type::i32, u8"i8_i32", env::MemoryType::i8To32);
	fMakeAccess(mod, state, wasm::Type::i32, u8"u16_i32", env::MemoryType::u16To32);
	fMakeAccess(mod, state, wasm::Type::i32, u8"i16_i32", env::MemoryType::i16To32);
	fMakeAccess(mod, state, wasm::Type::i32, u8"i32", env::MemoryType::i32);
	fMakeAccess(mod, state, wasm::Type::i64, u8"i64", env::MemoryType::i64);
	fMakeAccess(mod, state, wasm::Type::f32, u8"f32", env::MemoryType::f32);
	fMakeAccess(mod, state, wasm::Type::f64, u8"f64", env::MemoryType::f64);

	/* add the cache-flushing function */
	{
		wasm::Sink sink{ mod.function(u8"mem_flush_caches", {}, {}, wasm::Export{}) };

		/* destination-address */
		sink[I::U32::Const(pCacheAddress)];

		/* value */
		sink[I::U32::Const(0)];

		/* size */
		sink[I::U32::Const(uint64_t(pCacheCount + MemoryInteraction::InternalCaches) * sizeof(MemoryInteraction::MemCache))];

		sink[I::Memory::Fill(state.module.management)];
	}
}
void env::detail::MemoryInteraction::setupBlockImports(wasm::Module& mod, env::ModuleState& state) const {
	/* add the function-imports for the page-lookup */
	wasm::Prototype prototype = mod.prototype(u8"mem_lookup_usage_type", { { u8"addr", wasm::Type::i64 }, { u8"size", wasm::Type::i32 }, { u8"cache", wasm::Type::i32 } }, { wasm::Type::i32 });
	state.mem.read = mod.function(u8"lookup_read", prototype, wasm::Import{ u8"mem" });
	state.mem.write = mod.function(u8"lookup_write", prototype, wasm::Import{ u8"mem" });
	state.mem.execute = mod.function(u8"lookup_execute", prototype, wasm::Import{ u8"mem" });
}

void env::detail::MemoryInteraction::makeRead(const wasm::Variable& i64Address, const env::ModuleState& state, uint32_t cacheIndex, env::MemoryType type) const {
	fCheckCache(cacheIndex);
	return fMakeRead(i64Address.sink(), i64Address, state, cacheIndex, type);
}
void env::detail::MemoryInteraction::makeWrite(const wasm::Variable& i64Address, const wasm::Variable& value, const env::ModuleState& state, uint32_t cacheIndex, env::MemoryType type) const {
	fCheckCache(cacheIndex);
	return fMakeWrite(i64Address.sink(), i64Address, value, state, cacheIndex, type);
}
void env::detail::MemoryInteraction::makeExecute(const wasm::Variable& i64Address, const env::ModuleState& state, uint32_t cacheIndex, env::MemoryType type) const {
	fCheckCache(cacheIndex);
	return fMakeExecute(i64Address.sink(), i64Address, state, cacheIndex, type);
}
