#include "../generate.h"
#include "../environment/memory/env-memory.h"

static util::Logger logger{ u8"gen::memory" };

gen::detail::MemoryWriter::MemoryWriter(const detail::MemoryState& state) : pState{ state } {}

void gen::detail::MemoryWriter::fCheckCache(uint32_t cache) const {
	uint32_t caches = env::detail::MemoryAccess::CacheCount();
	if (cache >= caches)
		logger.fatal(u8"Cache [", cache, u8"] out of bounds as only [", caches, u8"] caches have been defined");
}
gen::detail::MemoryWriter::Access gen::detail::MemoryWriter::fGetVariables() {
	/* check if new temporary variables need to be created */
	if (pReserved >= pAccess.size()) {
		wasm::Variable address = gen::Sink->local(wasm::Type::i64, str::lu8<32>::Build(u8"_mem_address", pAccess.size()));
		wasm::Variable offset = gen::Sink->local(wasm::Type::i64, str::lu8<32>::Build(u8"_mem_offset", pAccess.size()));
		pAccess.push_back({ address, offset });
	}
	return pAccess[pReserved];
}

void gen::detail::MemoryWriter::fMakeRead(uint32_t cache, gen::MemoryType type, env::guest_t address, const wasm::Function* code) {
	uintptr_t cacheAddress = env::detail::MemoryAccess::CacheAddress() + cache * sizeof(env::detail::MemoryCache);
	if (type >= gen::MemoryType::_last)
		return;

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
	default:
		break;
	}

	/* fetch the temporary variables to be used */
	Access vars = fGetVariables();

	/* cache the current address */
	gen::Add[I::Local::Tee(vars.address)];

	/* compute the offset into the current cached region */
	gen::Add[I::U32::Const(cacheAddress)];
	gen::Add[I::U64::Load(pState.memory, offsetof(env::detail::MemoryCache, address))];
	gen::Add[I::U64::Sub()];
	gen::Add[I::Local::Tee(vars.offset)];

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
	default:
		break;
	}
	gen::Add[I::U64::LessEqual()];

	{
		/* less-equal: the value lies in range */
		wasm::IfThen _if{ gen::Sink, u8"", {}, { result } };

		/* compute the final absolute address */
		gen::Add[I::Local::Get(vars.offset)];
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
		default:
			break;
		}

		/* greater: a cache lookup needs to be performed */
		_if.otherwise();

		/* write the parameter to the stack */
		gen::Add[I::U64::Const(address)];
		gen::Add[I::Local::Get(vars.address)];
		gen::Add[I::U32::Const(cache)];

		/* perform the call to patch the cache (will leave the value on the stack) */
		if (code == 0)
			gen::Add[I::Call::Direct(pState.reads[size_t(type)])];
		else
			gen::Add[I::Call::Direct(*code)];
	}
}
void gen::detail::MemoryWriter::fMakeStartWrite(uint32_t cache, gen::MemoryType type, env::guest_t address) {
	/* perform this initial preparation of the value (to ensure that the initial i32 is on the stack) */

	/* check if the default-read cache is to be used and compute the actual cache address */
	uintptr_t cacheAddress = env::detail::MemoryAccess::CacheAddress() + cache * sizeof(env::detail::MemoryCache);
	if (type >= gen::MemoryType::_last)
		return;

	/* fetch the temporary variables to be used and update the reserved-counter until the final write is performed */
	Access vars = fGetVariables();
	++pReserved;

	/* cache the current address */
	gen::Add[I::Local::Tee(vars.address)];

	/* compute the offset into the current cached region */
	gen::Add[I::U32::Const(cacheAddress)];
	gen::Add[I::U64::Load(pState.memory, offsetof(env::detail::MemoryCache, address))];
	gen::Add[I::U64::Sub()];
	gen::Add[I::Local::Tee(vars.offset)];

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
	default:
		break;
	}
	gen::Add[I::U64::LessEqual()];

}
void gen::detail::MemoryWriter::fMakeStopWrite(uint32_t cache, gen::MemoryType type, env::guest_t address) {
	/* address is already partially computed and condition lies on the stack */

	/* fetch the top-most variables - which correspond to this write */
	Access vars = pAccess[--pReserved];

	/* check if the default-write cache is to be used and compute the actual cache address */
	uintptr_t cacheAddress = env::detail::MemoryAccess::CacheAddress() + cache * sizeof(env::detail::MemoryCache);
	if (type >= gen::MemoryType::_last)
		return;

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
		gen::Add[I::Local::Get(vars.offset)];
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
		default:
			break;
		}

		/* greater: a cache lookup needs to be performed */
		_if.otherwise();

		/* write the parameter to the stack */
		gen::Add[I::U64::Const(address)];
		gen::Add[I::Local::Get(vars.address)];
		gen::Add[I::U32::Const(cache)];
		gen::Add[I::Local::Get(*value)];

		/* perform the call to patch the cache (will automatically write the value) */
		gen::Add[I::Call::Direct(pState.writes[size_t(type)])];
	}
}

void gen::detail::MemoryWriter::makeRead(uint32_t cacheIndex, gen::MemoryType type, env::guest_t address) {
	fCheckCache(cacheIndex);
	fMakeRead(cacheIndex + env::detail::MemoryAccess::StartOfReadCaches(), type, address, 0);
}
void gen::detail::MemoryWriter::makeStartWrite(uint32_t cacheIndex, gen::MemoryType type, env::guest_t address) {
	fCheckCache(cacheIndex);
	fMakeStartWrite(cacheIndex + env::detail::MemoryAccess::StartOfWriteCaches(), type, address);
}
void gen::detail::MemoryWriter::makeEndWrite(uint32_t cacheIndex, gen::MemoryType type, env::guest_t address) {
	fMakeStopWrite(cacheIndex + env::detail::MemoryAccess::StartOfWriteCaches(), type, address);
}
