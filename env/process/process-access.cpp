#include "../env-process.h"

uint32_t env::detail::ProcessAccess::physicalPages() const {
	return env::Instance()->pPhysicalPages;
}
uint32_t env::detail::ProcessAccess::managementPages() const {
	return env::Instance()->pManagementPages;
}
