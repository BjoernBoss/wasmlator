#include "../env-process.h"

void env::Context::fCheck(uint32_t size) const {
	if (size > uint32_t(pBuffer.size()))
		host::Fatal(u8"Cannot access [", size, u8"] bytes from context of size [", pBuffer.size(), u8']');
}
void env::Context::fTerminate(int32_t code) {
	throw env::Termianted{ code };
}
void env::Context::fNotDecodable(env::guest_t address) {
	throw env::NotDecodable{ address };
}
void env::Context::fNotReachable(env::guest_t address) {
	host::Fatal(str::u8::Format(u8"Execution reached address [{:#018x}] which was considered not reachable by super-block", address));
}
