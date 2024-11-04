#include "../env-process.h"
#include "../../interface/interface.h"

void env::detail::ContextBridge::SetExitCode(int32_t code) {
	env::Instance()->context().fSetExitCode(code);
}

void env::detail::ContextBridge::Read(uint32_t offset, uint8_t* data, uint32_t size) {
	ctx_read(offset, data, size);
}
void env::detail::ContextBridge::Write(uint32_t offset, const uint8_t* data, uint32_t size) {
	ctx_write(offset, data, size);
}
