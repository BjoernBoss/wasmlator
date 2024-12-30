#include "../generate.h"

gen::detail::MappingWriter::MappingWriter(const detail::MappingState& state) : pState{ state } {}

void gen::detail::MappingWriter::makeGetFunction() const {
	gen::Add[I::Call::Direct(pState.lookup)];
	gen::Add[I::Table::Get(pState.functions)];
}
void gen::detail::MappingWriter::makeDirectInvoke() const {
	gen::Add[I::Call::Direct(pState.lookup)];
	gen::Add[I::Call::Indirect(pState.functions, pState.blockPrototype)];
}
void gen::detail::MappingWriter::makeTailInvoke() const {
	gen::Add[I::Call::Direct(pState.lookup)];
	gen::Add[I::Call::IndirectTail(pState.functions, pState.blockPrototype)];
}
const wasm::Prototype& gen::detail::MappingWriter::blockPrototype() const {
	return pState.blockPrototype;
}
