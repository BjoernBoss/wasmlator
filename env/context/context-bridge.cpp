#include "../env-process.h"
#include "../../interface/interface.h"

void env::bridge::Context::SetExitCode(uint64_t process, int32_t code) {
	reinterpret_cast<env::Process*>(process)->context().fSetExitCode(code);
}

void env::bridge::Context::Read(env::id_t id, uint32_t offset, uint8_t* data, uint32_t size) {
	ctx_read(id, offset, data, size);
}
void env::bridge::Context::Write(env::id_t id, uint32_t offset, const uint8_t* data, uint32_t size) {
	ctx_write(id, offset, data, size);
}
