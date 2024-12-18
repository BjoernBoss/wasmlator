#include "process-writer.h"

namespace I = wasm::inst;

gen::detail::ProcessWriter::ProcessWriter(const detail::ProcessState& state, wasm::Sink& sink) : pState{ state }, pSink{ sink } {}

void gen::detail::ProcessWriter::makeTerminate(env::guest_t address) const {
	pSink[I::U64::Const(address)];
	pSink[I::Call::Direct(pState.terminate)];
}
void gen::detail::ProcessWriter::makeNotDecodable(env::guest_t address) const {
	pSink[I::U64::Const(address)];
	pSink[I::Call::Direct(pState.notDecodable)];
}
void gen::detail::ProcessWriter::makeNotReachable(env::guest_t address) const {
	pSink[I::U64::Const(address)];
	pSink[I::Call::Direct(pState.notReachable)];
}
void gen::detail::ProcessWriter::makeSingleStep() const {
	pSink[I::Call::Direct(pState.singleStep)];
}
