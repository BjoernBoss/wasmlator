#include "../env-process.h"

env::detail::ContextAccess::ContextAccess(env::Process* process) : pProcess{ process } {}
uint32_t env::detail::ContextAccess::configureAndAllocate(uint32_t address, uint32_t context) {
	/* allocate the cache-entries from the management memory */
	pProcess->context().pAddress = address;
	pProcess->context().pSize = context;
	return context;
}
uint32_t env::detail::ContextAccess::contextAddress() const {
	return pProcess->context().pAddress;
}
uint32_t env::detail::ContextAccess::contextSize() const {
	return pProcess->context().pSize;
}
