#include "env-context.h"

static host::Logger logger{ u8"env::context" };

void env::Context::fCheck(uint32_t size) const {
	if (size > uint32_t(pBuffer.size()))
		logger.fatal(u8"Cannot access [", size, u8"] bytes from context of size [", pBuffer.size(), u8']');
}
void env::Context::fTerminate(int32_t code, env::guest_t address) {
	throw env::Terminated{ address, code };
}
void env::Context::fNotDecodable(env::guest_t address) {
	throw env::NotDecodable{ address };
}
void env::Context::fNotReachable(env::guest_t address) {
	logger.fatal(u8"Execution reached address [", str::As{ U"#018x", address }, u8"] which was considered not reachable by super-block");
}
