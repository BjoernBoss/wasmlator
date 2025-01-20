#include "../generate.h"
#include "../environment/memory/env-memory.h"

static util::Logger logger{ u8"gen::memory" };

gen::detail::MemoryWriter::MemoryWriter(const detail::MemoryState& state) : pState{ state } {}

void gen::detail::MemoryWriter::fCheckCache(uint32_t cache) const {
	uint32_t caches = uint32_t(env::detail::MemoryAccess::CacheCount());
	if (cache >= caches)
		logger.fatal(u8"Cache [", cache, u8"] out of bounds as only [", caches, u8"] caches have been defined");
}
uint32_t gen::detail::MemoryWriter::fMakeIndex(uint32_t cache, gen::MemoryType type) const {
	uint32_t index = cache * 4;

	/* add the size-offset to the index */
	switch (type) {
	case gen::MemoryType::u8To32:
	case gen::MemoryType::i8To32:
	case gen::MemoryType::u8To64:
	case gen::MemoryType::i8To64:
		index += 0;
		break;
	case gen::MemoryType::u16To32:
	case gen::MemoryType::i16To32:
	case gen::MemoryType::u16To64:
	case gen::MemoryType::i16To64:
		index += 1;
		break;
	case gen::MemoryType::u32To64:
	case gen::MemoryType::i32To64:
	case gen::MemoryType::i32:
	case gen::MemoryType::f32:
		index += 2;
		break;
	case gen::MemoryType::i64:
	case gen::MemoryType::f64:
		index += 3;
		break;
	}
	return index;
}
void gen::detail::MemoryWriter::fMakeAddress(uint32_t cache, const wasm::Function& lookup, gen::MemoryType type, env::guest_t address) const {
	uintptr_t cacheAddress = env::detail::MemoryAccess::CacheAddress() + cache * sizeof(env::detail::MemoryCache);

	/* check if the temporary variable needs to be initialized */
	if (!pAccess.valid())
		pAccess = gen::Sink->local(wasm::Type::i64, u8"_mem_access");

	/* compute the offset into the current cached region */
	gen::Add[I::U32::Const(cacheAddress)];
	gen::Add[I::U64::Load(pState.memory, offsetof(env::detail::MemoryCache, address))];
	gen::Add[I::U64::Sub()];
	gen::Add[I::Local::Tee(pAccess)];

	/* check if the accessed-address lies in the range */
	gen::Add[I::U32::Const(cacheAddress)];
	switch (type) {
	case gen::MemoryType::u8To32:
	case gen::MemoryType::u8To64:
	case gen::MemoryType::i8To32:
	case gen::MemoryType::i8To64:
		gen::Add[I::U64::Load32(pState.memory, offsetof(env::detail::MemoryCache, size1))];
		break;
	case gen::MemoryType::u16To32:
	case gen::MemoryType::u16To64:
	case gen::MemoryType::i16To32:
	case gen::MemoryType::i16To64:
		gen::Add[I::U64::Load32(pState.memory, offsetof(env::detail::MemoryCache, size2))];
		break;
	case gen::MemoryType::i32:
	case gen::MemoryType::f32:
	case gen::MemoryType::u32To64:
	case gen::MemoryType::i32To64:
		gen::Add[I::U64::Load32(pState.memory, offsetof(env::detail::MemoryCache, size4))];
		break;
	case gen::MemoryType::i64:
	case gen::MemoryType::f64:
		gen::Add[I::U64::Load32(pState.memory, offsetof(env::detail::MemoryCache, size8))];
		break;
	}
	gen::Add[I::U64::GreaterEqual()];

	{
		/* greater-equal, perform mem-cache lookup */
		wasm::IfThen _if{ gen::Sink, u8"", {}, { wasm::Type::i32 } };

		/* write the address to the stack */
		gen::Add[I::U64::Const(address)];

		/* recover the original accessed address */
		gen::Add[I::Local::Get(pAccess)];
		gen::Add[I::U32::Const(cacheAddress)];
		gen::Add[I::U64::Load(pState.memory, offsetof(env::detail::MemoryCache, address))];
		gen::Add[I::U64::Add()];

		/* perform the call to patch the cache (leaves the new absolute address as i32 on the stack) */
		gen::Add[I::Call::Direct(lookup)];

		/* less-equal, compute the final absolute address */
		_if.otherwise();
		gen::Add[I::Local::Get(pAccess)];
		gen::Add[I::U64::Shrink()];
		gen::Add[I::U32::Const(cacheAddress)];
		gen::Add[I::U32::Load(pState.memory, offsetof(env::detail::MemoryCache, physical))];
		gen::Add[I::U32::Add()];
	}
}
void gen::detail::MemoryWriter::fMakeRead(uint32_t cache, gen::MemoryType type, env::guest_t address, const wasm::Function* lookup) const {
	/* add the address-lookup */
	if (lookup != 0)
		fMakeAddress(cache, *lookup, type, address);
	else
		fMakeAddress(cache, pState.reads[fMakeIndex(cache, type)], type, address);

	/* add the final read-instruction */
	switch (type) {
	case gen::MemoryType::u8To32:
		gen::Add[I::U32::Load8(pState.physical)];
		break;
	case gen::MemoryType::u16To32:
		gen::Add[I::U32::Load16(pState.physical)];
		break;
	case gen::MemoryType::u8To64:
		gen::Add[I::U64::Load8(pState.physical)];
		break;
	case gen::MemoryType::u16To64:
		gen::Add[I::U64::Load16(pState.physical)];
		break;
	case gen::MemoryType::u32To64:
		gen::Add[I::U64::Load32(pState.physical)];
		break;
	case gen::MemoryType::i8To32:
		gen::Add[I::I32::Load8(pState.physical)];
		break;
	case gen::MemoryType::i16To32:
		gen::Add[I::I32::Load16(pState.physical)];
		break;
	case gen::MemoryType::i8To64:
		gen::Add[I::I64::Load8(pState.physical)];
		break;
	case gen::MemoryType::i16To64:
		gen::Add[I::I64::Load16(pState.physical)];
		break;
	case gen::MemoryType::i32To64:
		gen::Add[I::I64::Load32(pState.physical)];
		break;
	case gen::MemoryType::i32:
		gen::Add[I::U32::Load(pState.physical)];
		break;
	case gen::MemoryType::i64:
		gen::Add[I::U64::Load(pState.physical)];
		break;
	case gen::MemoryType::f32:
		gen::Add[I::F32::Load(pState.physical)];
		break;
	case gen::MemoryType::f64:
		gen::Add[I::F64::Load(pState.physical)];
		break;
	}
}
void gen::detail::MemoryWriter::fMakeWrite(uint32_t cache, gen::MemoryType type, env::guest_t address, const wasm::Function* lookup) const {
	wasm::Variable value;

	/* fetch the variable to be used for caching */
	switch (type) {
	case gen::MemoryType::u8To32:
	case gen::MemoryType::i8To32:
	case gen::MemoryType::u16To32:
	case gen::MemoryType::i16To32:
	case gen::MemoryType::i32:
		if (!pValuei32.valid())
			pValuei32 = gen::Sink->local(wasm::Type::i32, u8"_mem_i32");
		value = pValuei32;
		break;
	case gen::MemoryType::u8To64:
	case gen::MemoryType::i8To64:
	case gen::MemoryType::u16To64:
	case gen::MemoryType::i16To64:
	case gen::MemoryType::u32To64:
	case gen::MemoryType::i32To64:
	case gen::MemoryType::i64:
		if (!pValuei64.valid())
			pValuei64 = gen::Sink->local(wasm::Type::i64, u8"_mem_i64");
		value = pValuei64;
		break;
	case gen::MemoryType::f32:
		if (!pValuef32.valid())
			pValuef32 = gen::Sink->local(wasm::Type::f32, u8"_mem_f32");
		value = pValuef32;
		break;
	case gen::MemoryType::f64:
		if (!pValuef64.valid())
			pValuef64 = gen::Sink->local(wasm::Type::f64, u8"_mem_f64");
		value = pValuef64;
		break;
	}

	/* cache the value (as it must be placed after the address) and prepare the stack */
	gen::Add[I::Local::Set(value)];
	if (lookup != 0)
		fMakeAddress(cache, *lookup, type, address);
	else
		fMakeAddress(cache, pState.writes[fMakeIndex(cache, type)], type, address);
	gen::Add[I::Local::Get(value)];

	/* add the final store-instruction */
	switch (type) {
	case gen::MemoryType::u8To32:
	case gen::MemoryType::i8To32:
		gen::Add[I::U32::Store8(pState.physical)];
		break;
	case gen::MemoryType::u16To32:
	case gen::MemoryType::i16To32:
		gen::Add[I::U32::Store16(pState.physical)];
		break;
	case gen::MemoryType::u8To64:
	case gen::MemoryType::i8To64:
		gen::Add[I::U64::Store8(pState.physical)];
		break;
	case gen::MemoryType::u16To64:
	case gen::MemoryType::i16To64:
		gen::Add[I::U64::Store16(pState.physical)];
		break;
	case gen::MemoryType::u32To64:
	case gen::MemoryType::i32To64:
		gen::Add[I::U64::Store32(pState.physical)];
		break;
	case gen::MemoryType::i32:
		gen::Add[I::U32::Store(pState.physical)];
		break;
	case gen::MemoryType::i64:
		gen::Add[I::U64::Store(pState.physical)];
		break;
	case gen::MemoryType::f32:
		gen::Add[I::F32::Store(pState.physical)];
		break;
	case gen::MemoryType::f64:
		gen::Add[I::F64::Store(pState.physical)];
		break;
	}
}

void gen::detail::MemoryWriter::makeRead(uint32_t cacheIndex, gen::MemoryType type, env::guest_t address) const {
	fCheckCache(cacheIndex);
	fMakeRead(cacheIndex, type, address, 0);
}
void gen::detail::MemoryWriter::makeWrite(uint32_t cacheIndex, gen::MemoryType type, env::guest_t address) const {
	fCheckCache(cacheIndex);
	fMakeWrite(cacheIndex, type, address, 0);
}
