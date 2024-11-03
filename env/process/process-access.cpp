#include "../env-process.h"

env::detail::ProcessAccess::ProcessAccess(env::Process* process) : pProcess{ process } {}
uint32_t env::detail::ProcessAccess::physicalPages() const {
	return pProcess->pPhysicalPages;
}
uint32_t env::detail::ProcessAccess::managementPages() const {
	return pProcess->pManagementPages;
}
