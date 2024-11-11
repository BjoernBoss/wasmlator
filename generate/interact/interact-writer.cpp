#include "interact-writer.h"

namespace I = wasm::inst;

gen::detail::InteractWriter::InteractWriter(const detail::InteractState& state, wasm::Sink& sink) : pState{ state }, pSink{ sink } {}

void gen::detail::InteractWriter::makeVoid(uint32_t index) const {
	pSink[I::U32::Const(index)];
	pSink[I::Call::Direct(pState.invokeVoid)];
}
void gen::detail::InteractWriter::makeParam(uint32_t index) const {
	pSink[I::U32::Const(index)];
	pSink[I::Call::Direct(pState.invokeParam)];
}
