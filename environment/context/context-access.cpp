#include "../env-process.h"

uint32_t env::detail::ContextAccess::ConfigureAndAllocate(uint32_t address) {
	/* allocate the cache-entries from the management memory */
	env::Instance()->context().pAddress = address;
	env::Instance()->context().pSize = env::Instance()->specification().contextSize();
	return env::Instance()->context().pSize;
}
uint32_t env::detail::ContextAccess::ContextAddress() {
	return env::Instance()->context().pAddress;
}
uint32_t env::detail::ContextAccess::ContextSize() {
	return env::Instance()->context().pSize;
}
