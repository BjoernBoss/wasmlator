#include "../env-process.h"
#include "../../interface/interface.h"

void env::detail::ContextBridge::SetExitCode(int32_t code) {
	env::Instance()->context().fSetExitCode(code);
}

uint64_t env::detail::ContextBridge::Read(uint32_t offset, uint32_t size) {
	return ctx_read(offset, size);
}
void env::detail::ContextBridge::Write(uint32_t offset, uint32_t size, uint64_t value) {
	ctx_write(offset, size, value);
}
