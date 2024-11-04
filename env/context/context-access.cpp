#include "../env-process.h"

uint32_t env::detail::ContextAccess::configureAndAllocate(uint32_t address, uint32_t context) {
	/* allocate the cache-entries from the management memory */
	env::Instance()->context().pAddress = address;
	env::Instance()->context().pSize = context;
	return context;
}
uint32_t env::detail::ContextAccess::contextAddress() const {
	return env::Instance()->context().pAddress;
}
uint32_t env::detail::ContextAccess::contextSize() const {
	return env::Instance()->context().pSize;
}
