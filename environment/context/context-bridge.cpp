#include "../environment.h"
#include "../../host/interface.h"

void env::detail::ContextBridge::Terminate(int32_t code, uint64_t address) {
	env::Instance()->context().fTerminate(code, address);
}
void env::detail::ContextBridge::CodeException(uint64_t address, uint32_t id) {
	env::Instance()->context().fCodeException(address, static_cast<detail::CodeExceptions>(id));
}
