#include "mapping-writer.h"

namespace I = wasm::inst;

gen::detail::MappingWriter::MappingWriter(const detail::MappingState& state, wasm::Sink& sink) : pState{ state }, pSink{ sink } {}

void gen::detail::MappingWriter::makeGetFunction() const {
	pSink[I::Call::Direct(pState.lookup)];
	pSink[I::Table::Get(pState.functions)];
}
void gen::detail::MappingWriter::makeDirectInvoke() const {
	pSink[I::Call::Direct(pState.lookup)];
	pSink[I::Call::Indirect(pState.functions, {}, { wasm::Type::i64, wasm::Type::i32 })];
}
void gen::detail::MappingWriter::makeTailInvoke() const {
	pSink[I::Call::Direct(pState.lookup)];
	pSink[I::Call::IndirectTail(pState.functions, {}, { wasm::Type::i64, wasm::Type::i32 })];
}
