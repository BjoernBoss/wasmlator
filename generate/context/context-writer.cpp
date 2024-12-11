#include "context-writer.h"
#include "../environment/context/context-access.h"

static host::Logger logger{ u8"gen::context" };

namespace I = wasm::inst;

gen::detail::ContextWriter::ContextWriter(const detail::ContextState& state, wasm::Sink& sink) : pState{ state }, pSink{ sink } {}

void gen::detail::ContextWriter::fCheckRange(uint32_t offset, gen::MemoryType type) const {
	switch (type) {
	case gen::MemoryType::u8To32:
	case gen::MemoryType::u8To64:
	case gen::MemoryType::i8To32:
	case gen::MemoryType::i8To64:
		offset += 1;
		break;
	case gen::MemoryType::u16To32:
	case gen::MemoryType::u16To64:
	case gen::MemoryType::i16To32:
	case gen::MemoryType::i16To64:
		offset += 2;
		break;
	case gen::MemoryType::i32:
	case gen::MemoryType::f32:
	case gen::MemoryType::u32To64:
	case gen::MemoryType::i32To64:
		offset += 4;
		break;
	case gen::MemoryType::i64:
	case gen::MemoryType::f64:
		offset += 8;
		break;
	}

	uint32_t size = uint32_t(env::detail::ContextAccess::ContextSize());
	if (offset > size)
		logger.fatal(u8"Cannot read [", offset, u8"] bytes from context of size [", size, u8']');
}

void gen::detail::ContextWriter::makeTerminate(env::guest_t address) const {
	pSink[I::U32::Const(address)];
	pSink[I::Call::Direct(pState.terminate)];
}
void gen::detail::ContextWriter::makeRead(uint32_t offset, gen::MemoryType type) const {
	fCheckRange(offset, type);

	/* write the offset to the stack */
	pSink[I::U32::Const(env::detail::ContextAccess::ContextAddress() + offset)];

	/* read the value onto the stack */
	switch (type) {
	case gen::MemoryType::u8To32:
		pSink[I::U32::Load8(pState.memory)];
		break;
	case gen::MemoryType::u16To32:
		pSink[I::U32::Load16(pState.memory)];
		break;
	case gen::MemoryType::u8To64:
		pSink[I::U64::Load8(pState.memory)];
		break;
	case gen::MemoryType::u16To64:
		pSink[I::U64::Load16(pState.memory)];
		break;
	case gen::MemoryType::u32To64:
		pSink[I::U64::Load32(pState.memory)];
		break;
	case gen::MemoryType::i8To32:
		pSink[I::I32::Load8(pState.memory)];
		break;
	case gen::MemoryType::i16To32:
		pSink[I::I32::Load16(pState.memory)];
		break;
	case gen::MemoryType::i8To64:
		pSink[I::I64::Load8(pState.memory)];
		break;
	case gen::MemoryType::i16To64:
		pSink[I::I64::Load16(pState.memory)];
		break;
	case gen::MemoryType::i32To64:
		pSink[I::I64::Load32(pState.memory)];
		break;
	case gen::MemoryType::i32:
		pSink[I::U32::Load(pState.memory)];
		break;
	case gen::MemoryType::i64:
		pSink[I::U64::Load(pState.memory)];
		break;
	case gen::MemoryType::f32:
		pSink[I::F32::Load(pState.memory)];
		break;
	case gen::MemoryType::f64:
		pSink[I::F64::Load(pState.memory)];
		break;
	}
}
void gen::detail::ContextWriter::makeWrite(uint32_t offset, gen::MemoryType type) const {
	fCheckRange(offset, type);

	wasm::Variable value;

	/* fetch the variable to be used for caching */
	switch (type) {
	case gen::MemoryType::u8To32:
	case gen::MemoryType::i8To32:
	case gen::MemoryType::u16To32:
	case gen::MemoryType::i16To32:
	case gen::MemoryType::i32:
		if (!pValuei32.valid())
			pValuei32 = pSink.local(wasm::Type::i32, u8"_ctx_i32");
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
			pValuei64 = pSink.local(wasm::Type::i64, u8"_ctx_i64");
		value = pValuei64;
		break;
	case gen::MemoryType::f32:
		if (!pValuef32.valid())
			pValuef32 = pSink.local(wasm::Type::f32, u8"_ctx_f32");
		value = pValuef32;
		break;
	case gen::MemoryType::f64:
		if (!pValuef64.valid())
			pValuef64 = pSink.local(wasm::Type::f64, u8"_ctx_f64");
		value = pValuef64;
		break;
	}

	/* cache the value (as it must be placed after the address) and prepare the stack */
	pSink[I::Local::Set(value)];
	pSink[I::U32::Const(env::detail::ContextAccess::ContextAddress() + offset)];
	pSink[I::Local::Get(value)];

	/* write the value to the context */
	switch (type) {
	case gen::MemoryType::u8To32:
	case gen::MemoryType::i8To32:
		pSink[I::U32::Store8(pState.memory)];
		break;
	case gen::MemoryType::u16To32:
	case gen::MemoryType::i16To32:
		pSink[I::U32::Store16(pState.memory)];
		break;
	case gen::MemoryType::u8To64:
	case gen::MemoryType::i8To64:
		pSink[I::U64::Store8(pState.memory)];
		break;
	case gen::MemoryType::u16To64:
	case gen::MemoryType::i16To64:
		pSink[I::U64::Store16(pState.memory)];
		break;
	case gen::MemoryType::u32To64:
	case gen::MemoryType::i32To64:
		pSink[I::U64::Store32(pState.memory)];
		break;
	case gen::MemoryType::i32:
		pSink[I::U32::Store(pState.memory)];
		break;
	case gen::MemoryType::i64:
		pSink[I::U64::Store(pState.memory)];
		break;
	case gen::MemoryType::f32:
		pSink[I::F32::Store(pState.memory)];
		break;
	case gen::MemoryType::f64:
		pSink[I::F64::Store(pState.memory)];
		break;
	}
}
