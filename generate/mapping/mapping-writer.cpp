#include "mapping-writer.h"

namespace I = wasm::inst;

gen::detail::MappingWriter::MappingWriter(const detail::MappingState& state, wasm::Sink& sink) : pState{ state }, pSink{ sink } {}

void gen::detail::MappingWriter::makeCheckFailed() const {
	pSink[I::U32::EqualZero()];
}
void gen::detail::MappingWriter::makeLookup() const {
	pSink[I::Call::Direct(pState.lookup)];
}
void gen::detail::MappingWriter::makeLoadFunction() const {
	pSink[I::Table::Get(pState.functions)];
}
void gen::detail::MappingWriter::makeInvoke() const {
	pSink[I::Call::Indirect(pState.functions, {}, { wasm::Type::i64, wasm::Type::i32 })];
}
void gen::detail::MappingWriter::makeTailInvoke() const {
	pSink[I::Call::IndirectTail(pState.functions, {}, { wasm::Type::i64, wasm::Type::i32 })];
}
