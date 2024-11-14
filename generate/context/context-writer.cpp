#include "context-writer.h"

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

	uint32_t size = env::detail::ContextAccess::ContextSize();
	if (offset > size)
		host::Fatal(u8"Cannot read [", offset, u8"] bytes from context of size [", size, u8']');
}

void gen::detail::ContextWriter::makeExit() const {
	pSink[I::Call::Direct(pState.exit)];

	/* return from the block */
	pSink[I::U64::Const(0)];
	pSink[I::U32::Const(env::ExecState::terminated)];
	pSink[I::Return()];
}
void gen::detail::ContextWriter::makeRead(uint32_t offset, gen::MemoryType type) const {
	fCheckRange(offset, type);

	/* write the offset to the stack */
	pSink[I::U32::Const(env::detail::ContextAccess::ContextAddress() + offset)];

	/* read the value onto the stack */
	switch (type) {
	case gen::MemoryType::u8To32:
		pSink[I::U32::Load8(pState.management)];
		break;
	case gen::MemoryType::u16To32:
		pSink[I::U32::Load16(pState.management)];
		break;
	case gen::MemoryType::u8To64:
		pSink[I::U64::Load8(pState.management)];
		break;
	case gen::MemoryType::u16To64:
		pSink[I::U64::Load16(pState.management)];
		break;
	case gen::MemoryType::u32To64:
		pSink[I::U64::Load32(pState.management)];
		break;
	case gen::MemoryType::i8To32:
		pSink[I::I32::Load8(pState.management)];
		break;
	case gen::MemoryType::i16To32:
		pSink[I::I32::Load16(pState.management)];
		break;
	case gen::MemoryType::i8To64:
		pSink[I::I64::Load8(pState.management)];
		break;
	case gen::MemoryType::i16To64:
		pSink[I::I64::Load16(pState.management)];
		break;
	case gen::MemoryType::i32To64:
		pSink[I::I64::Load32(pState.management)];
		break;
	case gen::MemoryType::i32:
		pSink[I::U32::Load(pState.management)];
		break;
	case gen::MemoryType::i64:
		pSink[I::U64::Load(pState.management)];
		break;
	case gen::MemoryType::f32:
		pSink[I::F32::Load(pState.management)];
		break;
	case gen::MemoryType::f64:
		pSink[I::F64::Load(pState.management)];
		break;
	}
}
void gen::detail::ContextWriter::makeWrite(const wasm::Variable& value, uint32_t offset, gen::MemoryType type) const {
	fCheckRange(offset, type);

	/* write the address and value to the stack */
	pSink[I::U32::Const(env::detail::ContextAccess::ContextAddress() + offset)];
	pSink[I::Local::Get(value)];

	/* write the value to the context */
	switch (type) {
	case gen::MemoryType::u8To32:
	case gen::MemoryType::i8To32:
		pSink[I::U32::Store8(pState.management)];
		break;
	case gen::MemoryType::u16To32:
	case gen::MemoryType::i16To32:
		pSink[I::U32::Store16(pState.management)];
		break;
	case gen::MemoryType::u8To64:
	case gen::MemoryType::i8To64:
		pSink[I::U64::Store8(pState.management)];
		break;
	case gen::MemoryType::u16To64:
	case gen::MemoryType::i16To64:
		pSink[I::U64::Store16(pState.management)];
		break;
	case gen::MemoryType::u32To64:
	case gen::MemoryType::i32To64:
		pSink[I::U64::Store32(pState.management)];
		break;
	case gen::MemoryType::i32:
		pSink[I::U32::Store(pState.management)];
		break;
	case gen::MemoryType::i64:
		pSink[I::U64::Store(pState.management)];
		break;
	case gen::MemoryType::f32:
		pSink[I::F32::Store(pState.management)];
		break;
	case gen::MemoryType::f64:
		pSink[I::F64::Store(pState.management)];
		break;
	}
}
