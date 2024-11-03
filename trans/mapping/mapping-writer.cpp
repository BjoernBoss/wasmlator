#include "mapping-writer.h"

namespace I = wasm::inst;

void trans::detail::MappingWriter::makeLookup() const {
	pSink[I::Call::Tail(pState.lookup)];
}
void trans::detail::MappingWriter::makeInvoke() const {
	pSink[I::Call::Indirect(pState.functions, {}, { wasm::Type::i64, wasm::Type::i32 })];
}
