#include "env-context.h"

static host::Logger logger{ u8"env::context" };

void env::Context::fCheck(uint32_t size) const {
	if (size > uint32_t(pBuffer.size()))
		logger.fatal(u8"Cannot access [", size, u8"] bytes from context of size [", pBuffer.size(), u8']');
}
