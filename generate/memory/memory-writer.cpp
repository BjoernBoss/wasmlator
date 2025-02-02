#include "../generate.h"
#include "../environment/memory/env-memory.h"

static util::Logger logger{ u8"gen::memory" };

gen::detail::MemoryWriter::MemoryWriter(const detail::MemoryState& state) : pState{ state } {}

void gen::detail::MemoryWriter::fCheckCache(uint32_t cache) const {
	uint32_t caches = env::detail::MemoryAccess::CacheCount();
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
void gen::detail::MemoryWriter::fMakeRead(uint32_t cache, gen::MemoryType type, env::guest_t address, const wasm::Function* lookup) const {
	/* check if the default-read cache is to be used and compute the actual cache address */
	uint32_t cacheIndex = cache;
	if (lookup == 0)
		cacheIndex += env::detail::MemoryAccess::StartOfReadCaches();
	uintptr_t cacheAddress = env::detail::MemoryAccess::CacheAddress() + cacheIndex * sizeof(env::detail::MemoryCache);

	/* extract the result-type */
	wasm::Type result = wasm::Type::i32;
	switch (type) {
	case gen::MemoryType::u8To32:
	case gen::MemoryType::u16To32:
	case gen::MemoryType::i8To32:
	case gen::MemoryType::i16To32:
	case gen::MemoryType::i32:
		break;
	case gen::MemoryType::u8To64:
	case gen::MemoryType::u16To64:
	case gen::MemoryType::u32To64:
	case gen::MemoryType::i8To64:
	case gen::MemoryType::i16To64:
	case gen::MemoryType::i32To64:
	case gen::MemoryType::i64:
		result = wasm::Type::i64;
		break;
	case gen::MemoryType::f32:
		result = wasm::Type::f32;
		break;
	case gen::MemoryType::f64:
		result = wasm::Type::f64;
		break;
	}

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
	gen::Add[I::U64::LessEqual()];

	{
		/* less-equal: the value lies in range */
		wasm::IfThen _if{ gen::Sink, u8"", {}, { result } };

		/* compute the final absolute address */
		gen::Add[I::Local::Get(pAccess)];
		gen::Add[I::U64::Shrink()];
		gen::Add[I::U32::Const(cacheAddress)];
		gen::Add[I::U32::Load(pState.memory, offsetof(env::detail::MemoryCache, physical))];
		gen::Add[I::U32::Add()];

		/* add the actual read-instruction */
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

		/* greater: a cache lookup needs to be performed */
		_if.otherwise();

		/* write the address to the stack */
		gen::Add[I::U64::Const(address)];

		/* recover the original accessed address */
		gen::Add[I::Local::Get(pAccess)];
		gen::Add[I::U32::Const(cacheAddress)];
		gen::Add[I::U64::Load(pState.memory, offsetof(env::detail::MemoryCache, address))];
		gen::Add[I::U64::Add()];

		/* perform the call to patch the cache (will leave the value on the stack) */
		if (lookup != 0)
			gen::Add[I::Call::Direct(*lookup)];
		else
			gen::Add[I::Call::Direct(pState.reads[fMakeIndex(cache, type)])];

		/* patch or reinterpret the read value to the expected value */
		switch (type) {
		case gen::MemoryType::u8To32:
		case gen::MemoryType::u16To32:
		case gen::MemoryType::i8To32:
		case gen::MemoryType::i16To32:
		case gen::MemoryType::i32:
			gen::Add[I::U64::Shrink()];
			break;
		case gen::MemoryType::u8To64:
		case gen::MemoryType::u16To64:
		case gen::MemoryType::u32To64:
		case gen::MemoryType::i8To64:
		case gen::MemoryType::i16To64:
		case gen::MemoryType::i32To64:
		case gen::MemoryType::i64:
			break;
		case gen::MemoryType::f32:
			gen::Add[I::U64::Shrink()];
			gen::Add[I::U32::AsFloat()];
			break;
		case gen::MemoryType::f64:
			gen::Add[I::U64::AsFloat()];
			break;
		}
	}
}
void gen::detail::MemoryWriter::fMakeStartWrite(uint32_t cache, gen::MemoryType type, env::guest_t address, const wasm::Function* lookup) const {
	/* perform this initial preparation of the value (to ensure that the initial i32 is on the stack) */

	/* check if the default-read cache is to be used and compute the actual cache address */
	uint32_t cacheIndex = cache;
	if (lookup == 0)
		cacheIndex += env::detail::MemoryAccess::StartOfWriteCaches();
	uintptr_t cacheAddress = env::detail::MemoryAccess::CacheAddress() + cacheIndex * sizeof(env::detail::MemoryCache);

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
	gen::Add[I::U64::LessEqual()];

}
void gen::detail::MemoryWriter::fMakeStopWrite(uint32_t cache, gen::MemoryType type, env::guest_t address, const wasm::Function* lookup) const {
	/* address is already partially computed and condition lies on the stack */

	/* check if the default-write cache is to be used and compute the actual cache address */
	uint32_t cacheIndex = cache;
	if (lookup == 0)
		cacheIndex += env::detail::MemoryAccess::StartOfWriteCaches();
	uintptr_t cacheAddress = env::detail::MemoryAccess::CacheAddress() + cacheIndex * sizeof(env::detail::MemoryCache);

	/* cache the value to be written */
	wasm::Variable* value = 0;
	switch (type) {
	case gen::MemoryType::u8To64:
	case gen::MemoryType::i8To64:
	case gen::MemoryType::u16To64:
	case gen::MemoryType::i16To64:
	case gen::MemoryType::u32To64:
	case gen::MemoryType::i32To64:
	case gen::MemoryType::i64:
		if (!pValuei64.valid())
			pValuei64 = gen::Sink->local(wasm::Type::i64, u8"_mem_i64");
		value = &pValuei64;
		break;
	case gen::MemoryType::f32:
		if (!pValuef32.valid())
			pValuef32 = gen::Sink->local(wasm::Type::f32, u8"_mem_f32");
		value = &pValuef32;
		break;
	case gen::MemoryType::f64:
		if (!pValuef64.valid())
			pValuef64 = gen::Sink->local(wasm::Type::f64, u8"_mem_f64");
		value = &pValuef64;
		break;
	case gen::MemoryType::u8To32:
	case gen::MemoryType::i8To32:
	case gen::MemoryType::u16To32:
	case gen::MemoryType::i16To32:
	case gen::MemoryType::i32:
		/* default to silence static analyzer */
	default:
		if (!pValuei32.valid())
			pValuei32 = gen::Sink->local(wasm::Type::i32, u8"_mem_i32");
		value = &pValuei32;
		break;

	}
	gen::Add[I::Local::Set(*value)];

	{
		/* less-equal: the value lies in range */
		wasm::IfThen _if{ gen::Sink };

		/* compute the final absolute address */
		gen::Add[I::Local::Get(pAccess)];
		gen::Add[I::U64::Shrink()];
		gen::Add[I::U32::Const(cacheAddress)];
		gen::Add[I::U32::Load(pState.memory, offsetof(env::detail::MemoryCache, physical))];
		gen::Add[I::U32::Add()];

		/* write the value to the stack */
		gen::Add[I::Local::Get(*value)];

		/* add the actual store-instruction */
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

		/* greater: a cache lookup needs to be performed */
		_if.otherwise();

		/* write the address to the stack */
		gen::Add[I::U64::Const(address)];

		/* recover the original accessed address */
		gen::Add[I::Local::Get(pAccess)];
		gen::Add[I::U32::Const(cacheAddress)];
		gen::Add[I::U64::Load(pState.memory, offsetof(env::detail::MemoryCache, address))];
		gen::Add[I::U64::Add()];

		/* write the value to the stack */
		gen::Add[I::Local::Get(*value)];

		/* patch or reinterpret the value to an i64 */
		switch (value->type()) {
		case wasm::Type::i32:
			gen::Add[I::U32::Expand()];
			break;
		case wasm::Type::f32:
			gen::Add[I::F32::AsInt()];
			gen::Add[I::U32::Expand()];
			break;
		case wasm::Type::f64:
			gen::Add[I::F64::AsInt()];
			break;
		case wasm::Type::i64:
			/* to silence static analyzer */
		default:
			break;
		}

		/* perform the call to patch the cache (will automatically write the value) */
		if (lookup != 0)
			gen::Add[I::Call::Direct(*lookup)];
		else
			gen::Add[I::Call::Direct(pState.writes[fMakeIndex(cache, type)])];
	}
}

void gen::detail::MemoryWriter::makeRead(uint32_t cacheIndex, gen::MemoryType type, env::guest_t address) const {
	fCheckCache(cacheIndex);
	fMakeRead(cacheIndex, type, address, 0);
}
void gen::detail::MemoryWriter::makeStartWrite(uint32_t cacheIndex, gen::MemoryType type, env::guest_t address) const {
	fCheckCache(cacheIndex);
	fMakeStartWrite(cacheIndex, type, address, 0);
}
void gen::detail::MemoryWriter::makeEndWrite(uint32_t cacheIndex, gen::MemoryType type, env::guest_t address) const {
	fMakeStopWrite(cacheIndex, type, address, 0);
}
