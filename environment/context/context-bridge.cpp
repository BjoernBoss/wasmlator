#include "../environment.h"
#include "../../interface/interface.h"

void env::detail::ContextBridge::Terminate(int32_t code, uint64_t address) {
	env::Instance()->context().fTerminate(code, address);
}
void env::detail::ContextBridge::NotDecodable(uint64_t address) {
	env::Instance()->context().fNotDecodable(address);
}
void env::detail::ContextBridge::NotReachable(uint64_t address) {
	env::Instance()->context().fNotReachable(address);
}
