#include "../generate.h"

gen::detail::InteractWriter::InteractWriter(const detail::InteractState& state) : pState{ state } {}

void gen::detail::InteractWriter::makeVoid(uint32_t index) const {
	gen::Add[I::U32::Const(index)];
	gen::Add[I::Call::Direct(pState.invokeVoid)];
}
void gen::detail::InteractWriter::makeParam(uint32_t index) const {
	gen::Add[I::U32::Const(index)];
	gen::Add[I::Call::Direct(pState.invokeParam)];
}
