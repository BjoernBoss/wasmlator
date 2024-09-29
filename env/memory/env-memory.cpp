#include "env-memory.h"
#include "bridge-memory.h"

namespace I = wasm::inst;

env::Memory::Memory(env::Context& context, uint32_t cacheSize) : pContext{ &context }, pCacheCount{ cacheSize } {
	pReadCache = pCacheCount + 0;
	pWriteCache = pCacheCount + 1;
	pExecuteCache = pCacheCount + 2;
	pCachePages = env::PageCount((uint64_t(pCacheCount) + 3) * sizeof(Memory::MemCache));
}

void env::Memory::fCheckCache(uint32_t cache) const {
	if (cache >= pCacheCount)
		pContext->fail(u8"Cache [", cache, u8"] out of bounds as only [", pCacheCount, u8"] caches have been defined");
}
void env::Memory::fMakeAddress(wasm::Sink& sink, const env::MemoryState& state, uint32_t cache, const wasm::Variable& i64Address, const wasm::Function& lookup, env::MemoryType type) const {
	/* compute the offset into the current cached region */
	sink[I::Local::Get(i64Address)];
	sink[I::U32::Const(cache * sizeof(Memory::MemCache))];
	sink[I::U64::Load(state.caches, offsetof(Memory::MemCache, address))];
	sink[I::U64::Sub()];
	sink[I::Local::Tee(i64Address)];

	/* check if the address lies in the range */
	sink[I::U32::Const(cache * sizeof(Memory::MemCache))];
	switch (type) {
	case env::MemoryType::u8To32:
	case env::MemoryType::u8To64:
	case env::MemoryType::i8To32:
	case env::MemoryType::i8To64:
		sink[I::U64::Load32(state.caches, offsetof(Memory::MemCache, size1))];
		break;
	case env::MemoryType::u16To32:
	case env::MemoryType::u16To64:
	case env::MemoryType::i16To32:
	case env::MemoryType::i16To64:
		sink[I::U64::Load32(state.caches, offsetof(Memory::MemCache, size2))];
		break;
	case env::MemoryType::i32:
	case env::MemoryType::f32:
	case env::MemoryType::u32To64:
	case env::MemoryType::i32To64:
		sink[I::U64::Load32(state.caches, offsetof(Memory::MemCache, size4))];
		break;
	case env::MemoryType::i64:
	case env::MemoryType::f64:
		sink[I::U64::Load32(state.caches, offsetof(Memory::MemCache, size8))];
		break;
	}
	sink[I::U64::GreaterEqual()];

	{
		/* greater-equal, perform mem-cache lookup */
		wasm::IfThen _if{ sink, {}, state.ifElsePrototype };

		/* recover the original address */
		sink[I::Local::Get(i64Address)];
		sink[I::U32::Const(cache * sizeof(Memory::MemCache))];
		sink[I::U64::Load(state.caches, offsetof(Memory::MemCache, address))];
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
		sink[I::U32::Const(cache * sizeof(Memory::MemCache))];
		sink[I::Call::Direct(lookup)];

		/* less-equal, compute the final absolute address */
		_if.otherwise();
		sink[I::Local::Get(i64Address)];
		sink[I::U64::Shrink()];
		sink[I::U32::Const(cache * sizeof(Memory::MemCache))];
		sink[I::U32::Load(state.caches, offsetof(Memory::MemCache, physical))];
		sink[I::U32::Add()];
	}
}
void env::Memory::fMakeLookup(const wasm::Memory& caches, const wasm::Function& function, const wasm::Function& lookup, uint32_t uasge) const {
	wasm::Sink sink{ function };
	wasm::Variable address = sink.parameter(0), size = sink.parameter(1), cache = sink.parameter(2);

	/* allocate the local variables necessary to write the temporary result back */
	wasm::Variable outAddr = sink.local(wasm::Type::i64, u8"out_address");
	wasm::Variable outPhys = sink.local(wasm::Type::i32, u8"out_physical");
	wasm::Variable outSize = sink.local(wasm::Type::i32, u8"out_size");

	/* perform the call */
	sink[I::U64::Const(this)];
	sink[I::Local::Get(address)];
	sink[I::Local::Get(size)];
	sink[I::U32::Const(Memory::Usage::Read)];
	sink[I::Call::Direct(lookup)];

	/* store the result to the locals */
	sink[I::Local::Set(outSize)];
	sink[I::Local::Set(outPhys)];
	sink[I::Local::Set(outAddr)];

	/* write the address and physical offset back */
	sink[I::Local::Get(cache)];
	sink[I::Local::Get(outAddr)];
	sink[I::U64::Store(caches, offsetof(Memory::MemCache, address))];
	sink[I::Local::Get(cache)];
	sink[I::Local::Get(outPhys)];
	sink[I::U32::Store(caches, offsetof(Memory::MemCache, physical))];

	/* write the different sizes back */
	for (uint32_t i = 1; i <= 8; i *= 2) {
		sink[I::Local::Get(cache)];
		sink[I::Local::Get(outSize)];
		sink[I::U32::Const(i)];
		sink[I::U32::Sub()];
		switch (i) {
		case 1:
			sink[I::U32::Store(caches, offsetof(Memory::MemCache, size1))];
			break;
		case 2:
			sink[I::U32::Store(caches, offsetof(Memory::MemCache, size2))];
			break;
		case 4:
			sink[I::U32::Store(caches, offsetof(Memory::MemCache, size4))];
			break;
		case 8:
			sink[I::U32::Store(caches, offsetof(Memory::MemCache, size8))];
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
void env::Memory::fMakeRead(wasm::Sink& sink, const wasm::Variable& i64Address, const env::MemoryState& state, uint32_t cache, env::MemoryType type) const {
	fMakeAddress(sink, state, cache, i64Address, state.readFunction, type);

	/* add the final read-instruction */
	switch (type) {
	case env::MemoryType::u8To32:
		sink[I::U32::Load8(state.memory)];
		break;
	case env::MemoryType::u16To32:
		sink[I::U32::Load16(state.memory)];
		break;
	case env::MemoryType::u8To64:
		sink[I::U64::Load8(state.memory)];
		break;
	case env::MemoryType::u16To64:
		sink[I::U64::Load16(state.memory)];
		break;
	case env::MemoryType::u32To64:
		sink[I::U64::Load32(state.memory)];
		break;
	case env::MemoryType::i8To32:
		sink[I::I32::Load8(state.memory)];
		break;
	case env::MemoryType::i16To32:
		sink[I::I32::Load16(state.memory)];
		break;
	case env::MemoryType::i8To64:
		sink[I::I64::Load8(state.memory)];
		break;
	case env::MemoryType::i16To64:
		sink[I::I64::Load16(state.memory)];
		break;
	case env::MemoryType::i32To64:
		sink[I::I64::Load32(state.memory)];
		break;
	case env::MemoryType::i32:
		sink[I::U32::Load(state.memory)];
		break;
	case env::MemoryType::i64:
		sink[I::U64::Load(state.memory)];
		break;
	case env::MemoryType::f32:
		sink[I::F32::Load(state.memory)];
		break;
	case env::MemoryType::f64:
		sink[I::F64::Load(state.memory)];
		break;
	}
}
void env::Memory::fMakeWrite(wasm::Sink& sink, const wasm::Variable& i64Address, const wasm::Variable& value, const env::MemoryState& state, uint32_t cache, env::MemoryType type) const {
	fMakeAddress(sink, state, cache, i64Address, state.writeFunction, type);

	/* write the value to the stack */
	sink[I::Local::Get(value)];

	/* add the final store-instruction */
	switch (type) {
	case env::MemoryType::u8To32:
	case env::MemoryType::i8To32:
		sink[I::U32::Store8(state.memory)];
		break;
	case env::MemoryType::u16To32:
	case env::MemoryType::i16To32:
		sink[I::U32::Store16(state.memory)];
		break;
	case env::MemoryType::u8To64:
	case env::MemoryType::i8To64:
		sink[I::U64::Store8(state.memory)];
		break;
	case env::MemoryType::u16To64:
	case env::MemoryType::i16To64:
		sink[I::U64::Store16(state.memory)];
		break;
	case env::MemoryType::u32To64:
	case env::MemoryType::i32To64:
		sink[I::U64::Store32(state.memory)];
		break;
	case env::MemoryType::i32:
		sink[I::U32::Store(state.memory)];
		break;
	case env::MemoryType::i64:
		sink[I::U64::Store(state.memory)];
		break;
	case env::MemoryType::f32:
		sink[I::F32::Store(state.memory)];
		break;
	case env::MemoryType::f64:
		sink[I::F64::Store(state.memory)];
		break;
	}
}
void env::Memory::fMakeExecute(wasm::Sink& sink, const wasm::Variable& i64Address, const env::MemoryState& state, uint32_t cache, env::MemoryType type) const {
	fMakeAddress(sink, state, cache, i64Address, state.executeFunction, type);

	/* add the final read-instruction */
	switch (type) {
	case env::MemoryType::u8To32:
		sink[I::U32::Load8(state.memory)];
		break;
	case env::MemoryType::u16To32:
		sink[I::U32::Load16(state.memory)];
		break;
	case env::MemoryType::u8To64:
		sink[I::U64::Load8(state.memory)];
		break;
	case env::MemoryType::u16To64:
		sink[I::U64::Load16(state.memory)];
		break;
	case env::MemoryType::u32To64:
		sink[I::U64::Load32(state.memory)];
		break;
	case env::MemoryType::i8To32:
		sink[I::I32::Load8(state.memory)];
		break;
	case env::MemoryType::i16To32:
		sink[I::I32::Load16(state.memory)];
		break;
	case env::MemoryType::i8To64:
		sink[I::I64::Load8(state.memory)];
		break;
	case env::MemoryType::i16To64:
		sink[I::I64::Load16(state.memory)];
		break;
	case env::MemoryType::i32To64:
		sink[I::I64::Load32(state.memory)];
		break;
	case env::MemoryType::i32:
		sink[I::U32::Load(state.memory)];
		break;
	case env::MemoryType::i64:
		sink[I::U64::Load(state.memory)];
		break;
	case env::MemoryType::f32:
		sink[I::F32::Load(state.memory)];
		break;
	case env::MemoryType::f64:
		sink[I::F64::Load(state.memory)];
		break;
	}
}
void env::Memory::fMakeAccess(wasm::Module& mod, const env::MemoryState& state, const wasm::Prototype& readPrototype, const wasm::Prototype& writePrototype, std::u8string_view name, env::MemoryType type) const {
	std::u8string readName = str::Build<std::u8string>(u8"mem_read_", name);
	std::u8string writeName = str::Build<std::u8string>(u8"mem_write_", name);
	std::u8string executeName = str::Build<std::u8string>(u8"mem_execute_", name);

	wasm::Sink read{ mod.function(readPrototype, readName, {}, wasm::Export{ readName }) };
	wasm::Sink write{ mod.function(writePrototype, writeName, {}, wasm::Export{ writeName }) };
	wasm::Sink execute{ mod.function(readPrototype, executeName, {}, wasm::Export{ executeName }) };

	/* make the read, write, and execute function */
	fMakeRead(read, read.parameter(0), state, pReadCache, type);
	fMakeExecute(execute, execute.parameter(0), state, pExecuteCache, type);
	fMakeWrite(write, write.parameter(0), write.parameter(1), state, pExecuteCache, type);
}

env::MemoryRegion env::Memory::fLookup(env::addr_t address, uint32_t size, uint32_t usage) {
	util::log(str::Format<std::u8string>(u8"Lookup [{:#16x}] with size [{}] and usage [{:04b}]", address, size, usage));
	return { 0, 0, 0 };
}
uint32_t env::Memory::fReadi32Fromi8(env::addr_t address) const {
	return bridge::Memory::Readi32Fromi8(pContext->id(), address);
}
uint32_t env::Memory::fReadi32Fromu8(env::addr_t address) const {
	return bridge::Memory::Readi32Fromu8(pContext->id(), address);
}
uint32_t env::Memory::fReadi32Fromi16(env::addr_t address) const {
	return bridge::Memory::Readi32Fromi16(pContext->id(), address);
}
uint32_t env::Memory::fReadi32Fromu16(env::addr_t address) const {
	return bridge::Memory::Readi32Fromu16(pContext->id(), address);
}
uint32_t env::Memory::fReadi32(env::addr_t address) const {
	return bridge::Memory::Readi32(pContext->id(), address);
}
uint64_t env::Memory::fReadi64(env::addr_t address) const {
	return bridge::Memory::Readi64(pContext->id(), address);
}
float env::Memory::fReadf32(env::addr_t address) const {
	return bridge::Memory::Readf32(pContext->id(), address);
}
double env::Memory::fReadf64(env::addr_t address) const {
	return bridge::Memory::Readf64(pContext->id(), address);
}
void env::Memory::fWritei32Fromi8(env::addr_t address, uint32_t value) const {
	bridge::Memory::Writei32Fromi8(pContext->id(), address, value);
}
void env::Memory::fWritei32Fromu8(env::addr_t address, uint32_t value) const {
	bridge::Memory::Writei32(pContext->id(), address, value);
}
void env::Memory::fWritei32Fromi16(env::addr_t address, uint32_t value) const {
	bridge::Memory::Writei32Fromi16(pContext->id(), address, value);
}
void env::Memory::fWritei32Fromu16(env::addr_t address, uint32_t value) const {
	bridge::Memory::Writei32Fromu16(pContext->id(), address, value);
}
void env::Memory::fWritei32(env::addr_t address, uint32_t value) const {
	bridge::Memory::Writei32(pContext->id(), address, value);
}
void env::Memory::fWritei64(env::addr_t address, uint64_t value) const {
	bridge::Memory::Writei64(pContext->id(), address, value);
}
void env::Memory::fWritef32(env::addr_t address, float value) const {
	bridge::Memory::Writef32(pContext->id(), address, value);
}
void env::Memory::fWritef64(env::addr_t address, double value) const {
	bridge::Memory::Writef64(pContext->id(), address, value);
}
uint32_t env::Memory::fExecutei32Fromi8(env::addr_t address) const {
	return bridge::Memory::Executei32Fromi8(pContext->id(), address);
}
uint32_t env::Memory::fExecutei32Fromu8(env::addr_t address) const {
	return bridge::Memory::Executei32Fromu8(pContext->id(), address);
}
uint32_t env::Memory::fExecutei32Fromi16(env::addr_t address) const {
	return bridge::Memory::Executei32Fromi16(pContext->id(), address);
}
uint32_t env::Memory::fExecutei32Fromu16(env::addr_t address) const {
	return bridge::Memory::Executei32Fromu16(pContext->id(), address);
}
uint32_t env::Memory::fExecutei32(env::addr_t address) const {
	return bridge::Memory::Executei32(pContext->id(), address);
}
uint64_t env::Memory::fExecutei64(env::addr_t address) const {
	return bridge::Memory::Executei64(pContext->id(), address);
}
float env::Memory::fExecutef32(env::addr_t address) const {
	return bridge::Memory::Executef32(pContext->id(), address);
}
double env::Memory::fExecutef64(env::addr_t address) const {
	return bridge::Memory::Executef64(pContext->id(), address);
}

env::MemoryState env::Memory::setupCoreModule(wasm::Module& mod) const {
	env::MemoryState state;

	/* add the import to the lookup-function */
	wasm::Prototype lookupPrototype = mod.prototype({
		{ u8"self", wasm::Type::i64 },
		{ u8"addr", wasm::Type::i64 },
		{ u8"size", wasm::Type::i32 },
		{ u8"usage", wasm::Type::i32 } },
		{ wasm::Type::i64, wasm::Type::i32, wasm::Type::i32 },
		u8"mem_lookup_type"
		);
	wasm::Function lookup = mod.function(lookupPrototype, u8"mem_import_lookup", wasm::Import{ u8"memory", u8"import_lookup" });

	/* add the core linear memory and page-lookup */
	state.memory = mod.memory(wasm::Limit{ env::MinPages }, u8"mem_core", {}, wasm::Export{ u8"mem_core" });
	state.caches = mod.memory(wasm::Limit{ pCachePages, pCachePages }, u8"mem_cache", {}, wasm::Export{ u8"mem_cache" });

	/* add the functions for the page-patching (receive the address as parameter and return the new absolute address) */
	wasm::Prototype prototype = mod.prototype({ { u8"addr", wasm::Type::i64 }, { u8"size", wasm::Type::i32 }, { u8"cache", wasm::Type::i32 } }, { wasm::Type::i32 }, u8"mem_addr_lookup");
	state.readFunction = mod.function(prototype, u8"mem_lookup_read", {}, wasm::Export{ u8"mem_lookup_read" });
	state.writeFunction = mod.function(prototype, u8"mem_lookup_write", {}, wasm::Export{ u8"mem_lookup_write" });
	state.executeFunction = mod.function(prototype, u8"mem_lookup_execute", {}, wasm::Export{ u8"mem_lookup_execute" });

	/* add the actual implementations */
	fMakeLookup(state.caches, state.readFunction, lookup, Memory::Usage::Read);
	fMakeLookup(state.caches, state.writeFunction, lookup, Memory::Usage::Write);
	fMakeLookup(state.caches, state.executeFunction, lookup, Memory::Usage::Execute);

	/* add the if-else prototype */
	state.ifElsePrototype = mod.prototype({}, { wasm::Type::i32 }, u8"mem_if_else");

	/* add the separate access functions */
	wasm::Prototype readi32 = mod.prototype({ { u8"addr", wasm::Type::i64 } }, { wasm::Type::i32 }, u8"mem_read_i32");
	wasm::Prototype readf32 = mod.prototype({ { u8"addr", wasm::Type::i64 } }, { wasm::Type::f32 }, u8"mem_read_f32");
	wasm::Prototype readi64 = mod.prototype({ { u8"addr", wasm::Type::i64 } }, { wasm::Type::i64 }, u8"mem_read_i64");
	wasm::Prototype readf64 = mod.prototype({ { u8"addr", wasm::Type::i64 } }, { wasm::Type::f64 }, u8"mem_read_f64");
	wasm::Prototype writei32 = mod.prototype({ { u8"addr", wasm::Type::i64 }, { u8"value", wasm::Type::i32 } }, {}, u8"mem_write_i32");
	wasm::Prototype writef32 = mod.prototype({ { u8"addr", wasm::Type::i64 }, { u8"value", wasm::Type::f32 } }, {}, u8"mem_write_f32");
	wasm::Prototype writei64 = mod.prototype({ { u8"addr", wasm::Type::i64 }, { u8"value", wasm::Type::i64 } }, {}, u8"mem_write_i64");
	wasm::Prototype writef64 = mod.prototype({ { u8"addr", wasm::Type::i64 }, { u8"value", wasm::Type::f64 } }, {}, u8"mem_write_f64");
	fMakeAccess(mod, state, readi32, writei32, u8"u8_i32", env::MemoryType::u8To32);
	fMakeAccess(mod, state, readi32, writei32, u8"i8_i32", env::MemoryType::i8To32);
	fMakeAccess(mod, state, readi32, writei32, u8"u16_i32", env::MemoryType::u16To32);
	fMakeAccess(mod, state, readi32, writei32, u8"i16_i32", env::MemoryType::i16To32);
	fMakeAccess(mod, state, readi32, writei32, u8"i32", env::MemoryType::i32);
	fMakeAccess(mod, state, readi64, writei64, u8"i64", env::MemoryType::i64);
	fMakeAccess(mod, state, readf32, writef32, u8"f32", env::MemoryType::f32);
	fMakeAccess(mod, state, readf64, writef64, u8"f64", env::MemoryType::f64);
	return state;
}
env::MemoryState env::Memory::setupImports(wasm::Module& mod) const {
	env::MemoryState state;

	/* add the core linear memory and cache-lookup imports */
	state.memory = mod.memory(wasm::Limit{ env::MinPages }, u8"mem_core", wasm::Import{ pContext->selfName(), u8"mem_core" });
	state.caches = mod.memory(wasm::Limit{ pCachePages, pCachePages }, u8"mem_cache", wasm::Import{ pContext->selfName(), u8"mem_cache" });

	/* add the function-imports for the page-lookup */
	wasm::Prototype prototype = mod.prototype({ { u8"addr", wasm::Type::i64 }, { u8"size", wasm::Type::i32 }, { u8"cache", wasm::Type::i32 } }, { wasm::Type::i32 }, u8"mem_addr_lookup");
	state.readFunction = mod.function(prototype, u8"mem_lookup_read", wasm::Import{ pContext->selfName(), u8"mem_lookup_read" });
	state.writeFunction = mod.function(prototype, u8"mem_lookup_write", wasm::Import{ pContext->selfName(), u8"mem_lookup_write" });
	state.executeFunction = mod.function(prototype, u8"mem_lookup_execute", wasm::Import{ pContext->selfName(), u8"mem_lookup_execute" });

	/* add the if-else prototype */
	state.ifElsePrototype = mod.prototype({}, { wasm::Type::i32 }, u8"mem_if_else");
	return state;
}
void env::Memory::makeRead(const wasm::Variable& i64Address, const env::MemoryState& state, uint32_t cache, env::MemoryType type) const {
	wasm::Sink& sink = wasm::Variable{ i64Address }.sink();
	fCheckCache(cache);
	return fMakeRead(sink, i64Address, state, cache, type);
}
void env::Memory::makeWrite(const wasm::Variable& i64Address, const wasm::Variable& value, const env::MemoryState& state, uint32_t cache, env::MemoryType type) const {
	wasm::Sink& sink = wasm::Variable{ i64Address }.sink();
	fCheckCache(cache);
	return fMakeWrite(sink, i64Address, value, state, cache, type);
}
void env::Memory::makeExecute(const wasm::Variable& i64Address, const env::MemoryState& state, uint32_t cache, env::MemoryType type) const {
	wasm::Sink& sink = wasm::Variable{ i64Address }.sink();
	fCheckCache(cache);
	return fMakeExecute(sink, i64Address, state, cache, type);
}
