#include "env-memory.h"

namespace I = wasm::inst;

namespace xchg {
	struct LookupInput {
		uint64_t process{ 0 };
		uint64_t address{ 0 };
		uint32_t size{ 0 };
		uint32_t usage{ 0 };
	};
	struct LookupOutput {
		uint64_t address{ 0 };
		uint32_t offset{ 0 };
		uint32_t size{ 0 };
	};
}

extern "C" static xchg::LookupOutput PerformLookup(xchg::LookupInput input) {
	env::MemoryRegion reg = reinterpret_cast<env::Process*>(input.process)->memory().lookup(input.address, input.size, uint32_t(input.usage));
	return { reg.address, reg.physical, reg.size };
}

env::Memory::Memory(env::Process* process, uint32_t cacheSize) : pCacheCount{ cacheSize } {
	pCachePages = env::PageCount(pCacheCount * sizeof(Memory::MemCache));
}

void env::Memory::fMakeAddress(wasm::Sink& sink, const env::MemoryState& state, uint32_t cache, const wasm::Variable& i64Address, const wasm::Function& lookup, env::MemoryType type) const {
	if (cache >= pCacheCount)
		pProcess->fail(u8"Cache [", cache, u8"] out of bounds as only [", pCacheCount, u8"] caches have been defined");

	/* compute the offset into the current cached region */
	sink[I::Local::Get(i64Address)];
	sink[I::U32::Const(cache * sizeof(Memory::MemCache))];
	sink[I::U64::Load(state.caches, offsetof(Memory::MemCache, address))];
	sink[I::U64::Sub()];
	sink[I::Local::Tee(i64Address)];

	/* check if the address lies in the range */
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
	case env::MemoryType::u32:
	case env::MemoryType::f32:
	case env::MemoryType::u32To64:
	case env::MemoryType::i32To64:
		sink[I::U64::Load32(state.caches, offsetof(Memory::MemCache, size4))];
		break;
	case env::MemoryType::u64:
	case env::MemoryType::f64:
		sink[I::U64::Load32(state.caches, offsetof(Memory::MemCache, size8))];
		break;
	}
	sink[I::U64::GreaterEqual()];

	{
		/* greater-equal, perform mem-cache lookup */
		wasm::IfThen _if{ sink };

		/* recover the original address */
		sink[I::Local::Get(i64Address)];
		sink[I::U32::Const(cache * sizeof(Memory::MemCache))];
		sink[I::U64::Load(state.caches, offsetof(Memory::MemCache, address))];
		sink[I::U64::Add()];

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
	sink[I::U64::Const(pProcess)];
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

env::MemoryState env::Memory::makeExport(wasm::Module& mod, const wasm::Function& lookup) const {
	env::MemoryState state;

	/* add the core linear memory and page-lookup */
	state.memory = mod.memory(wasm::Limit{ env::MinPages }, u8"core_memory", {}, wasm::Export{ u8"core_memory" });
	state.caches = mod.memory(wasm::Limit{ pCachePages, pCachePages }, u8"cache_memory", {}, wasm::Export{ u8"cache_memory" });

	/* add the functions for the page-patching (receive the address as parameter and return the new absolute address) */
	wasm::Prototype prototype = mod.prototype({ { u8"addr", wasm::Type::i64 }, { u8"size", wasm::Type::i32 }, { u8"cache", wasm::Type::i32 } }, { wasm::Type::i32 }, u8"addr_lookup");
	state.readFunction = mod.function(prototype, u8"lookup_read", {}, wasm::Export{ u8"lookup_read" });
	state.writeFunction = mod.function(prototype, u8"lookup_write", {}, wasm::Export{ u8"lookup_write" });
	state.executeFunction = mod.function(prototype, u8"lookup_execute", {}, wasm::Export{ u8"lookup_execute" });

	/* add the actual implementations */
	fMakeLookup(state.caches, state.readFunction, lookup, Memory::Usage::Read);
	fMakeLookup(state.caches, state.writeFunction, lookup, Memory::Usage::Write);
	fMakeLookup(state.caches, state.executeFunction, lookup, Memory::Usage::Execute);
}
env::MemoryState env::Memory::makeImport(wasm::Module& mod, std::u8string moduleName) const {
	env::MemoryState state;

	/* add the core linear memory and cache-lookup imports */
	state.memory = mod.memory(wasm::Limit{ env::MinPages }, u8"core_memory", wasm::Import{ moduleName, u8"core_memory" });
	state.caches = mod.memory(wasm::Limit{ pCachePages, pCachePages }, u8"cache_memory", wasm::Import{ moduleName, u8"cache_memory" });

	/* add the function-imports for the page-lookup */
	wasm::Prototype prototype = mod.prototype({ { u8"addr", wasm::Type::i64 }, { u8"size", wasm::Type::i32 }, { u8"cache", wasm::Type::i32 } }, { wasm::Type::i32 }, u8"addr_lookup");
	state.readFunction = mod.function(prototype, u8"lookup_read", wasm::Import{ moduleName, u8"lookup_read" });
	state.writeFunction = mod.function(prototype, u8"lookup_write", wasm::Import{ moduleName, u8"lookup_write" });
	state.executeFunction = mod.function(prototype, u8"lookup_execute", wasm::Import{ moduleName, u8"lookup_execute" });
	return state;
}
void env::Memory::makeRead(const wasm::Variable& i64Address, const env::MemoryState& state, uint32_t cache, env::MemoryType type) const {
	wasm::Sink& sink = wasm::Variable{ i64Address }.sink();
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
	case env::MemoryType::u32:
		sink[I::U32::Load(state.memory)];
		break;
	case env::MemoryType::u64:
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
void env::Memory::makeWrite(const wasm::Variable& i64Address, const env::MemoryState& state, uint32_t cache, env::MemoryType type) const {
	wasm::Sink& sink = wasm::Variable{ i64Address }.sink();
	fMakeAddress(sink, state, cache, i64Address, state.writeFunction, type);

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
	case env::MemoryType::u32:
		sink[I::U32::Store(state.memory)];
		break;
	case env::MemoryType::u64:
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
void env::Memory::makeExecute(const wasm::Variable& i64Address, const env::MemoryState& state, uint32_t cache, env::MemoryType type) const {
	wasm::Sink& sink = wasm::Variable{ i64Address }.sink();
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
	case env::MemoryType::u32:
		sink[I::U32::Load(state.memory)];
		break;
	case env::MemoryType::u64:
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
