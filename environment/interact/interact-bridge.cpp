#include "../env-process.h"
#include "../../interface/interface.h"

void env::detail::InteractBridge::InvokeVoid(uint32_t index) {
	env::Instance()->interact().fInvoke(index);
}
uint64_t env::detail::InteractBridge::InvokeParam(uint64_t param, uint32_t index) {
	return env::Instance()->interact().fInvoke(param, index);
}

void env::detail::InteractBridge::CallVoid(uint32_t index) {
	int_call_void(index);
}
uint64_t env::detail::InteractBridge::CallParam(uint64_t param, uint32_t index) {
	return int_call_param(param, index);
}
