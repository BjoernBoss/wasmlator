#include "mapping-writer.h"

namespace I = wasm::inst;

trans::detail::MappingWriter::MappingWriter(const detail::MappingState& state, wasm::Sink& sink) : pState{ state }, pSink{ sink } {}

void trans::detail::MappingWriter::makeCheckFailed() const {
	pSink[I::U32::EqualZero()];
}
void trans::detail::MappingWriter::makeLookup() const {
	pSink[I::Call::Direct(pState.lookup)];
}
void trans::detail::MappingWriter::makeLoadFunction() const {
	pSink[I::Table::Get(pState.functions)];
}
void trans::detail::MappingWriter::makeInvoke() const {
	pSink[I::Call::Indirect(pState.functions, {}, { wasm::Type::i64, wasm::Type::i32 })];
}
void trans::detail::MappingWriter::makeTailInvoke() const {
	pSink[I::Call::IndirectTail(pState.functions, {}, { wasm::Type::i64, wasm::Type::i32 })];
}
