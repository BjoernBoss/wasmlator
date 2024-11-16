#include "../env-process.h"
#include "../../interface/interface.h"

void env::detail::ContextBridge::Terminate(int32_t code) {
	env::Instance()->context().fTerminate(code);
}
void env::detail::ContextBridge::NotDecodable(uint64_t address) {
	env::Instance()->context().fNotDecodable(address);
}

uint64_t env::detail::ContextBridge::Read(uint32_t offset, uint32_t size) {
	return ctx_read(offset, size);
}
void env::detail::ContextBridge::Write(uint32_t offset, uint32_t size, uint64_t value) {
	ctx_write(offset, size, value);
}
