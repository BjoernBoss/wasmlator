#include "../env-process.h"

env::Context::Context(env::Process* process) : pProcess{ process } {}

void env::Context::fRead(uint32_t offset, uint8_t* data, uint32_t size) {
	if (offset + size >= pSize)
		pProcess->fail(u8"Cannot read [", offset + size, u8"] bytes from context of size [", pSize, u8']');
	bridge::Context::Read(pProcess->id(), offset, data, size);
}
void env::Context::fWrite(uint32_t offset, const uint8_t* data, uint32_t size) {
	if (offset + size >= pSize)
		pProcess->fail(u8"Cannot write [", offset + size, u8"] bytes from context of size [", pSize, u8']');
	bridge::Context::Write(pProcess->id(), offset, data, size);
}
void env::Context::fSetExitCode(int32_t code) {
	pExit = code;
}

int32_t env::Context::exitCode() const {
	return pExit;
}
