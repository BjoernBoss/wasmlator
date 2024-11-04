#include "context-writer.h"

namespace I = wasm::inst;

trans::detail::ContextWriter::ContextWriter(const detail::ContextState& state, wasm::Sink& sink) : pState{ state }, pSink{ sink } {}

void trans::detail::ContextWriter::fCheckRange(uint32_t offset, trans::MemoryType type) const {
	switch (type) {
	case trans::MemoryType::u8To32:
	case trans::MemoryType::u8To64:
	case trans::MemoryType::i8To32:
	case trans::MemoryType::i8To64:
		offset += 1;
		break;
	case trans::MemoryType::u16To32:
	case trans::MemoryType::u16To64:
	case trans::MemoryType::i16To32:
	case trans::MemoryType::i16To64:
		offset += 2;
		break;
	case trans::MemoryType::i32:
	case trans::MemoryType::f32:
	case trans::MemoryType::u32To64:
	case trans::MemoryType::i32To64:
		offset += 4;
		break;
	case trans::MemoryType::i64:
	case trans::MemoryType::f64:
		offset += 8;
		break;
	}

	uint32_t size = env::detail::ContextAccess{}.contextSize();
	if (offset > size)
		host::Fail(u8"Cannot read [", offset, u8"] bytes from context of size [", size, u8']');
}

void trans::detail::ContextWriter::makeExit() const {
	pSink[I::Call::Direct(pState.exit)];

	/* return from the block */
	pSink[I::U64::Const(0)];
	pSink[I::U32::Const(env::ExecState::terminated)];
	pSink[I::Return()];
}
void trans::detail::ContextWriter::makeRead(uint32_t offset, trans::MemoryType type) const {
	fCheckRange(offset, type);

	/* write the offset to the stack */
	pSink[I::U32::Const(env::detail::ContextAccess{}.contextAddress() + offset)];

	/* read the value onto the stack */
	switch (type) {
	case trans::MemoryType::u8To32:
		pSink[I::U32::Load8(pState.management)];
		break;
	case trans::MemoryType::u16To32:
		pSink[I::U32::Load16(pState.management)];
		break;
	case trans::MemoryType::u8To64:
		pSink[I::U64::Load8(pState.management)];
		break;
	case trans::MemoryType::u16To64:
		pSink[I::U64::Load16(pState.management)];
		break;
	case trans::MemoryType::u32To64:
		pSink[I::U64::Load32(pState.management)];
		break;
	case trans::MemoryType::i8To32:
		pSink[I::I32::Load8(pState.management)];
		break;
	case trans::MemoryType::i16To32:
		pSink[I::I32::Load16(pState.management)];
		break;
	case trans::MemoryType::i8To64:
		pSink[I::I64::Load8(pState.management)];
		break;
	case trans::MemoryType::i16To64:
		pSink[I::I64::Load16(pState.management)];
		break;
	case trans::MemoryType::i32To64:
		pSink[I::I64::Load32(pState.management)];
		break;
	case trans::MemoryType::i32:
		pSink[I::U32::Load(pState.management)];
		break;
	case trans::MemoryType::i64:
		pSink[I::U64::Load(pState.management)];
		break;
	case trans::MemoryType::f32:
		pSink[I::F32::Load(pState.management)];
		break;
	case trans::MemoryType::f64:
		pSink[I::F64::Load(pState.management)];
		break;
	}
}
void trans::detail::ContextWriter::makeWrite(const wasm::Variable& value, uint32_t offset, trans::MemoryType type) const {
	fCheckRange(offset, type);

	/* write the address and value to the stack */
	pSink[I::U32::Const(env::detail::ContextAccess{}.contextAddress() + offset)];
	pSink[I::Local::Get(value)];

	/* write the value to the context */
	switch (type) {
	case trans::MemoryType::u8To32:
	case trans::MemoryType::i8To32:
		pSink[I::U32::Store8(pState.management)];
		break;
	case trans::MemoryType::u16To32:
	case trans::MemoryType::i16To32:
		pSink[I::U32::Store16(pState.management)];
		break;
	case trans::MemoryType::u8To64:
	case trans::MemoryType::i8To64:
		pSink[I::U64::Store8(pState.management)];
		break;
	case trans::MemoryType::u16To64:
	case trans::MemoryType::i16To64:
		pSink[I::U64::Store16(pState.management)];
		break;
	case trans::MemoryType::u32To64:
	case trans::MemoryType::i32To64:
		pSink[I::U64::Store32(pState.management)];
		break;
	case trans::MemoryType::i32:
		pSink[I::U32::Store(pState.management)];
		break;
	case trans::MemoryType::i64:
		pSink[I::U64::Store(pState.management)];
		break;
	case trans::MemoryType::f32:
		pSink[I::F32::Store(pState.management)];
		break;
	case trans::MemoryType::f64:
		pSink[I::F64::Store(pState.management)];
		break;
	}
}
