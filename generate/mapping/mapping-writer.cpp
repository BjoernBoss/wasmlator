#include "mapping-writer.h"

namespace I = wasm::inst;

gen::detail::MappingWriter::MappingWriter(const detail::MappingState& state, wasm::Sink& sink) : pState{ state }, pSink{ sink } {}

void gen::detail::MappingWriter::makeGetFunction() const {
	pSink[I::Call::Direct(pState.lookup)];
	pSink[I::Table::Get(pState.functions)];
}
void gen::detail::MappingWriter::makeDirectInvoke() const {
	pSink[I::Call::Direct(pState.lookup)];
	pSink[I::Call::Indirect(pState.functions, pState.blockPrototype)];
}
void gen::detail::MappingWriter::makeTailInvoke() const {
	pSink[I::Call::Direct(pState.lookup)];
	pSink[I::Call::IndirectTail(pState.functions, pState.blockPrototype)];
}
const wasm::Prototype& gen::detail::MappingWriter::blockPrototype() const {
	return pState.blockPrototype;
}
