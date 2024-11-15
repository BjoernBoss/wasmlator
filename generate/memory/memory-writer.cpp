#include "memory-writer.h"
#include "../environment/memory/env-memory.h"

namespace I = wasm::inst;

gen::detail::MemoryWriter::MemoryWriter(const detail::MemoryState& state, wasm::Sink& sink) : pState{ state }, pSink{ sink } {}

void gen::detail::MemoryWriter::fCheckCache(uint32_t cache) const {
	uint32_t caches = env::detail::MemoryAccess::CacheAddress();
	if (cache >= caches)
		host::Fatal(u8"Cache [", cache, u8"] out of bounds as only [", caches, u8"] caches have been defined");
}
void gen::detail::MemoryWriter::fMakeAddress(uint32_t cache, const wasm::Function& lookup, gen::MemoryType type) const {
	uint32_t cacheAddress = env::detail::MemoryAccess::CacheAddress();

	/* check if the temporary variable needs to be initialized */
	if (!pTempAddress.valid())
		pTempAddress = pSink.local(wasm::Type::i64, u8"mem_temp_address");

	/* compute the offset into the current cached region */
	pSink[I::U32::Const(cacheAddress + cache * sizeof(env::detail::MemoryCache))];
	pSink[I::U64::Load(pState.management, offsetof(env::detail::MemoryCache, address))];
	pSink[I::U64::Sub()];
	pSink[I::Local::Tee(pTempAddress)];

	/* check if the address lies in the range */
	pSink[I::U32::Const(cacheAddress + cache * sizeof(env::detail::MemoryCache))];
	switch (type) {
	case gen::MemoryType::u8To32:
	case gen::MemoryType::u8To64:
	case gen::MemoryType::i8To32:
	case gen::MemoryType::i8To64:
		pSink[I::U64::Load32(pState.management, offsetof(env::detail::MemoryCache, size1))];
		break;
	case gen::MemoryType::u16To32:
	case gen::MemoryType::u16To64:
	case gen::MemoryType::i16To32:
	case gen::MemoryType::i16To64:
		pSink[I::U64::Load32(pState.management, offsetof(env::detail::MemoryCache, size2))];
		break;
	case gen::MemoryType::i32:
	case gen::MemoryType::f32:
	case gen::MemoryType::u32To64:
	case gen::MemoryType::i32To64:
		pSink[I::U64::Load32(pState.management, offsetof(env::detail::MemoryCache, size4))];
		break;
	case gen::MemoryType::i64:
	case gen::MemoryType::f64:
		pSink[I::U64::Load32(pState.management, offsetof(env::detail::MemoryCache, size8))];
		break;
	}
	pSink[I::U64::GreaterEqual()];

	{
		/* greater-equal, perform mem-cache lookup */
		wasm::IfThen _if{ pSink, {}, {}, { wasm::Type::i32 } };

		/* recover the original address */
		pSink[I::Local::Get(pTempAddress)];
		pSink[I::U32::Const(cacheAddress + cache * sizeof(env::detail::MemoryCache))];
		pSink[I::U64::Load(pState.management, offsetof(env::detail::MemoryCache, address))];
		pSink[I::U64::Add()];

		/* write the current size to the stack */
		switch (type) {
		case gen::MemoryType::u8To32:
		case gen::MemoryType::u8To64:
		case gen::MemoryType::i8To32:
		case gen::MemoryType::i8To64:
			pSink[I::U32::Const(1)];
			break;
		case gen::MemoryType::u16To32:
		case gen::MemoryType::u16To64:
		case gen::MemoryType::i16To32:
		case gen::MemoryType::i16To64:
			pSink[I::U32::Const(2)];
			break;
		case gen::MemoryType::i32:
		case gen::MemoryType::f32:
		case gen::MemoryType::u32To64:
		case gen::MemoryType::i32To64:
			pSink[I::U32::Const(4)];
			break;
		case gen::MemoryType::i64:
		case gen::MemoryType::f64:
			pSink[I::U32::Const(8)];
			break;
		}

		/* perform the call to patch the cache (leaves the new absolute address as i32 on the stack) */
		pSink[I::U32::Const(cacheAddress + cache * sizeof(env::detail::MemoryCache))];
		pSink[I::Call::Direct(lookup)];

		/* less-equal, compute the final absolute address */
		_if.otherwise();
		pSink[I::Local::Get(pTempAddress)];
		pSink[I::U64::Shrink()];
		pSink[I::U32::Const(cacheAddress + cache * sizeof(env::detail::MemoryCache))];
		pSink[I::U32::Load(pState.management, offsetof(env::detail::MemoryCache, physical))];
		pSink[I::U32::Add()];
	}
}
void gen::detail::MemoryWriter::fMakeRead(uint32_t cache, gen::MemoryType type) const {
	fMakeAddress(cache, pState.read, type);

	/* add the final read-instruction */
	switch (type) {
	case gen::MemoryType::u8To32:
		pSink[I::U32::Load8(pState.physical)];
		break;
	case gen::MemoryType::u16To32:
		pSink[I::U32::Load16(pState.physical)];
		break;
	case gen::MemoryType::u8To64:
		pSink[I::U64::Load8(pState.physical)];
		break;
	case gen::MemoryType::u16To64:
		pSink[I::U64::Load16(pState.physical)];
		break;
	case gen::MemoryType::u32To64:
		pSink[I::U64::Load32(pState.physical)];
		break;
	case gen::MemoryType::i8To32:
		pSink[I::I32::Load8(pState.physical)];
		break;
	case gen::MemoryType::i16To32:
		pSink[I::I32::Load16(pState.physical)];
		break;
	case gen::MemoryType::i8To64:
		pSink[I::I64::Load8(pState.physical)];
		break;
	case gen::MemoryType::i16To64:
		pSink[I::I64::Load16(pState.physical)];
		break;
	case gen::MemoryType::i32To64:
		pSink[I::I64::Load32(pState.physical)];
		break;
	case gen::MemoryType::i32:
		pSink[I::U32::Load(pState.physical)];
		break;
	case gen::MemoryType::i64:
		pSink[I::U64::Load(pState.physical)];
		break;
	case gen::MemoryType::f32:
		pSink[I::F32::Load(pState.physical)];
		break;
	case gen::MemoryType::f64:
		pSink[I::F64::Load(pState.physical)];
		break;
	}
}
void gen::detail::MemoryWriter::fMakeCode(uint32_t cache, gen::MemoryType type) const {
	fMakeAddress(cache, pState.code, type);

	/* add the final read-instruction */
	switch (type) {
	case gen::MemoryType::u8To32:
		pSink[I::U32::Load8(pState.physical)];
		break;
	case gen::MemoryType::u16To32:
		pSink[I::U32::Load16(pState.physical)];
		break;
	case gen::MemoryType::u8To64:
		pSink[I::U64::Load8(pState.physical)];
		break;
	case gen::MemoryType::u16To64:
		pSink[I::U64::Load16(pState.physical)];
		break;
	case gen::MemoryType::u32To64:
		pSink[I::U64::Load32(pState.physical)];
		break;
	case gen::MemoryType::i8To32:
		pSink[I::I32::Load8(pState.physical)];
		break;
	case gen::MemoryType::i16To32:
		pSink[I::I32::Load16(pState.physical)];
		break;
	case gen::MemoryType::i8To64:
		pSink[I::I64::Load8(pState.physical)];
		break;
	case gen::MemoryType::i16To64:
		pSink[I::I64::Load16(pState.physical)];
		break;
	case gen::MemoryType::i32To64:
		pSink[I::I64::Load32(pState.physical)];
		break;
	case gen::MemoryType::i32:
		pSink[I::U32::Load(pState.physical)];
		break;
	case gen::MemoryType::i64:
		pSink[I::U64::Load(pState.physical)];
		break;
	case gen::MemoryType::f32:
		pSink[I::F32::Load(pState.physical)];
		break;
	case gen::MemoryType::f64:
		pSink[I::F64::Load(pState.physical)];
		break;
	}
}
void gen::detail::MemoryWriter::fMakeWrite(const wasm::Variable& value, uint32_t cache, gen::MemoryType type) const {
	fMakeAddress(cache, pState.write, type);

	/* write the value to the stack */
	pSink[I::Local::Get(value)];

	/* add the final store-instruction */
	switch (type) {
	case gen::MemoryType::u8To32:
	case gen::MemoryType::i8To32:
		pSink[I::U32::Store8(pState.physical)];
		break;
	case gen::MemoryType::u16To32:
	case gen::MemoryType::i16To32:
		pSink[I::U32::Store16(pState.physical)];
		break;
	case gen::MemoryType::u8To64:
	case gen::MemoryType::i8To64:
		pSink[I::U64::Store8(pState.physical)];
		break;
	case gen::MemoryType::u16To64:
	case gen::MemoryType::i16To64:
		pSink[I::U64::Store16(pState.physical)];
		break;
	case gen::MemoryType::u32To64:
	case gen::MemoryType::i32To64:
		pSink[I::U64::Store32(pState.physical)];
		break;
	case gen::MemoryType::i32:
		pSink[I::U32::Store(pState.physical)];
		break;
	case gen::MemoryType::i64:
		pSink[I::U64::Store(pState.physical)];
		break;
	case gen::MemoryType::f32:
		pSink[I::F32::Store(pState.physical)];
		break;
	case gen::MemoryType::f64:
		pSink[I::F64::Store(pState.physical)];
		break;
	}
}

void gen::detail::MemoryWriter::makeRead(uint32_t cacheIndex, gen::MemoryType type) const {
	fCheckCache(cacheIndex);
	fMakeRead(cacheIndex, type);
}
void gen::detail::MemoryWriter::makeCode(uint32_t cacheIndex, gen::MemoryType type) const {
	fCheckCache(cacheIndex);
	fMakeCode(cacheIndex, type);
}
void gen::detail::MemoryWriter::makeWrite(const wasm::Variable& value, uint32_t cacheIndex, gen::MemoryType type) const {
	fCheckCache(cacheIndex);
	fMakeWrite(value, cacheIndex, type);
}
