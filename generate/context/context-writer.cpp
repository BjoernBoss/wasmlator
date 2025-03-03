/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025 Bjoern Boss Henrichsen */
#include "../generate.h"
#include "../../environment/context/env-context.h"

static util::Logger logger{ u8"gen::context" };

gen::detail::ContextWriter::ContextWriter(const detail::ContextState& state) : pState{ state } {}

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
	default:
		break;
	}

	uint32_t size = uint32_t(env::detail::ContextAccess::ContextSize());
	if (offset > size)
		logger.fatal(u8"Cannot read [", offset, u8"] bytes from context of size [", size, u8']');
}
void gen::detail::ContextWriter::fMakeHostRead(uintptr_t host, gen::MemoryType type) const {
	/* write the offset to the stack */
	gen::Add[I::U32::Const(host)];

	/* read the value onto the stack */
	switch (type) {
	case gen::MemoryType::u8To32:
		gen::Add[I::U32::Load8(pState.memory)];
		break;
	case gen::MemoryType::u16To32:
		gen::Add[I::U32::Load16(pState.memory)];
		break;
	case gen::MemoryType::u8To64:
		gen::Add[I::U64::Load8(pState.memory)];
		break;
	case gen::MemoryType::u16To64:
		gen::Add[I::U64::Load16(pState.memory)];
		break;
	case gen::MemoryType::u32To64:
		gen::Add[I::U64::Load32(pState.memory)];
		break;
	case gen::MemoryType::i8To32:
		gen::Add[I::I32::Load8(pState.memory)];
		break;
	case gen::MemoryType::i16To32:
		gen::Add[I::I32::Load16(pState.memory)];
		break;
	case gen::MemoryType::i8To64:
		gen::Add[I::I64::Load8(pState.memory)];
		break;
	case gen::MemoryType::i16To64:
		gen::Add[I::I64::Load16(pState.memory)];
		break;
	case gen::MemoryType::i32To64:
		gen::Add[I::I64::Load32(pState.memory)];
		break;
	case gen::MemoryType::i32:
		gen::Add[I::U32::Load(pState.memory)];
		break;
	case gen::MemoryType::i64:
		gen::Add[I::U64::Load(pState.memory)];
		break;
	case gen::MemoryType::f32:
		gen::Add[I::F32::Load(pState.memory)];
		break;
	case gen::MemoryType::f64:
		gen::Add[I::F64::Load(pState.memory)];
		break;
	default:
		break;
	}
}
void gen::detail::ContextWriter::fMakeHostStartWrite(uintptr_t host) const {
	gen::Add[I::U32::Const(host)];
}
void gen::detail::ContextWriter::fMakeHostEndWrite(gen::MemoryType type) const {
	/* write the value to the context */
	switch (type) {
	case gen::MemoryType::u8To32:
	case gen::MemoryType::i8To32:
		gen::Add[I::U32::Store8(pState.memory)];
		break;
	case gen::MemoryType::u16To32:
	case gen::MemoryType::i16To32:
		gen::Add[I::U32::Store16(pState.memory)];
		break;
	case gen::MemoryType::u8To64:
	case gen::MemoryType::i8To64:
		gen::Add[I::U64::Store8(pState.memory)];
		break;
	case gen::MemoryType::u16To64:
	case gen::MemoryType::i16To64:
		gen::Add[I::U64::Store16(pState.memory)];
		break;
	case gen::MemoryType::u32To64:
	case gen::MemoryType::i32To64:
		gen::Add[I::U64::Store32(pState.memory)];
		break;
	case gen::MemoryType::i32:
		gen::Add[I::U32::Store(pState.memory)];
		break;
	case gen::MemoryType::i64:
		gen::Add[I::U64::Store(pState.memory)];
		break;
	case gen::MemoryType::f32:
		gen::Add[I::F32::Store(pState.memory)];
		break;
	case gen::MemoryType::f64:
		gen::Add[I::F64::Store(pState.memory)];
		break;
	default:
		break;
	}
}

void gen::detail::ContextWriter::makeRead(uint32_t offset, gen::MemoryType type) const {
	fCheckRange(offset, type);
	fMakeHostRead(env::detail::ContextAccess::ContextAddress() + offset, type);
}
void gen::detail::ContextWriter::makeStartWrite(uint32_t offset, gen::MemoryType type) const {
	fCheckRange(offset, type);
	fMakeHostStartWrite(env::detail::ContextAccess::ContextAddress() + offset);
}
void gen::detail::ContextWriter::makeEndWrite(gen::MemoryType type) const {
	fMakeHostEndWrite(type);
}
void gen::detail::ContextWriter::makeHostRead(const void* host, gen::MemoryType type) const {
	fMakeHostRead(reinterpret_cast<uintptr_t>(host), type);
}
void gen::detail::ContextWriter::makeStartHostWrite(void* host) const {
	fMakeHostStartWrite(reinterpret_cast<uintptr_t>(host));
}
void gen::detail::ContextWriter::makeEndHostWrite(gen::MemoryType type) const {
	fMakeHostEndWrite(type);
}
void gen::detail::ContextWriter::makeTerminate(env::guest_t address) const {
	gen::Add[I::U64::Const(address)];
	gen::Add[I::Call::Direct(pState.terminate)];
}
void gen::detail::ContextWriter::makeNotDecodable(env::guest_t address) const {
	gen::Add[I::U64::Const(address)];
	gen::Add[I::U32::Const(env::detail::CodeExceptions::notDecodable)];
	gen::Add[I::Call::Direct(pState.codeException)];
}
void gen::detail::ContextWriter::makeNotReadable(env::guest_t address) const {
	gen::Add[I::U64::Const(address)];
	gen::Add[I::U32::Const(env::detail::CodeExceptions::notReadable)];
	gen::Add[I::Call::Direct(pState.codeException)];
}
void gen::detail::ContextWriter::makeNotReachable(env::guest_t address) const {
	gen::Add[I::U64::Const(address)];
	gen::Add[I::U32::Const(env::detail::CodeExceptions::notReachable)];
	gen::Add[I::Call::Direct(pState.codeException)];
}
