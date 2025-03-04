/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025 Bjoern Boss Henrichsen */
#include "../generate.h"
#include "../../environment/environment.h"

static util::Logger logger{ u8"gen::memory" };

gen::detail::MemoryWriter::MemoryWriter(const detail::MemoryState& state) : pState{ state } {}

void gen::detail::MemoryWriter::fCheckCache(uint32_t cache) const {
	uint32_t caches = env::detail::MemoryAccess::CacheCount();
	if (cache >= caches)
		logger.fatal(u8"Cache [", cache, u8"] out of bounds as only [", caches, u8"] caches have been defined");
}

void gen::detail::MemoryWriter::fMakeRead(uint32_t cache, gen::MemoryType type, env::guest_t address, const wasm::Function* code) {
	uintptr_t cacheAddress = env::detail::MemoryAccess::CacheAddress() + cache * sizeof(env::detail::MemoryCache);
	if (type >= gen::MemoryType::_end)
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

	/* setup the temporary variables to be used */
	if (!pAddress.valid())
		pAddress = gen::Sink->local(wasm::Type::i64, u8"_mem_address");
	if (!pOffset.valid())
		pOffset = gen::Sink->local(wasm::Type::i64, u8"_mem_offset");

	/* cache the current address */
	gen::Add[I::Local::Tee(pAddress)];

	/* compute the offset into the current cached region */
	gen::Add[I::U32::Const(cacheAddress)];
	gen::Add[I::U64::Load(pState.memory, offsetof(env::detail::MemoryCache, address))];
	gen::Add[I::U64::Sub()];
	gen::Add[I::Local::Tee(pOffset)];

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
	gen::Add[I::U64::Less()];

	{
		/* less: the value lies in range */
		wasm::IfThen _if{ gen::Sink, u8"", {}, { result } };

		/* compute the final absolute address */
		gen::Add[I::Local::Get(pOffset)];
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

		/* greater-equal: a cache lookup needs to be performed */
		_if.otherwise();

		/* write the parameter to the stack */
		gen::Add[I::U64::Const(address)];
		gen::Add[I::Local::Get(pAddress)];
		gen::Add[I::U32::Const(cache)];

		/* perform the call to patch the cache (will leave the value on the stack) */
		if (code == 0)
			gen::Add[I::Call::Direct(pState.reads[size_t(type)])];
		else
			gen::Add[I::Call::Direct(*code)];
	}
}
void gen::detail::MemoryWriter::fMakeWrite(uint32_t cache, gen::MemoryType type, env::guest_t address, env::guest_t nextAddress) {
	/* perform the full operation in-place as intermediate operations might otherwise modify or corrupt the caches */

	/* compute the actual cache address */
	uintptr_t cacheAddress = env::detail::MemoryAccess::CacheAddress() + cache * sizeof(env::detail::MemoryCache);
	if (type >= gen::MemoryType::_end)
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

	/* setup the temporary variables to be used */
	if (!pAddress.valid())
		pAddress = gen::Sink->local(wasm::Type::i64, u8"_mem_address");
	if (!pOffset.valid())
		pOffset = gen::Sink->local(wasm::Type::i64, u8"_mem_offset");

	/* cache the current address */
	gen::Add[I::Local::Tee(pAddress)];

	/* compute the offset into the current cached region */
	gen::Add[I::U32::Const(cacheAddress)];
	gen::Add[I::U64::Load(pState.memory, offsetof(env::detail::MemoryCache, address))];
	gen::Add[I::U64::Sub()];
	gen::Add[I::Local::Tee(pOffset)];

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
	gen::Add[I::U64::Less()];

	{
		/* less: the value lies in range */
		wasm::IfThen _if{ gen::Sink };

		/* compute the final absolute address */
		gen::Add[I::Local::Get(pOffset)];
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

		/* greater-equal: a cache lookup needs to be performed */
		_if.otherwise();

		/* write the parameter to the stack */
		gen::Add[I::U64::Const(address)];
		gen::Add[I::Local::Get(pAddress)];
		gen::Add[I::U32::Const(cache)];
		gen::Add[I::Local::Get(*value)];

		/* check if the instruction-size needs to be supplied */
		if (env::Instance()->detectWriteExecute())
			gen::Add[I::U32::Const(nextAddress - address)];

		/* perform the call to patch the cache (will automatically write the value) */
		gen::Add[I::Call::Direct(pState.writes[size_t(type)])];
	}
}

void gen::detail::MemoryWriter::makeRead(uint32_t cacheIndex, gen::MemoryType type, env::guest_t address) {
	fCheckCache(cacheIndex);
	fMakeRead(cacheIndex + env::detail::MemoryAccess::StartOfReadCaches(), type, address, 0);
}
void gen::detail::MemoryWriter::makeWrite(uint32_t cacheIndex, gen::MemoryType type, env::guest_t address, env::guest_t nextAddress) {
	fCheckCache(cacheIndex);
	fMakeWrite(cacheIndex + env::detail::MemoryAccess::StartOfWriteCaches(), type, address, nextAddress);
}
