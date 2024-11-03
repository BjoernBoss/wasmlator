#include "mapping-writer.h"

namespace I = wasm::inst;

trans::detail::MappingWriter::MappingWriter(const detail::MappingState& state, env::Process* process, wasm::Sink& sink) : pState{ state }, pProcess{ process }, pSink{ sink } {}

void trans::detail::MappingWriter::makeLookup() const {
	pSink[I::Call::Direct(pState.lookup)];
}
void trans::detail::MappingWriter::makeFuncLoad() const {
	pSink[I::Table::Get(pState.functions)];
}
void trans::detail::MappingWriter::makeInvoke() const {
	pSink[I::Call::Indirect(pState.functions, {}, { wasm::Type::i64, wasm::Type::i32 })];
}
