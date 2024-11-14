#include "../env-process.h"

uint32_t env::detail::ProcessAccess::PhysicalPages() {
	return env::Instance()->pPhysicalPages;
}
uint32_t env::detail::ProcessAccess::ManagementPages() {
	return env::Instance()->pManagementPages;
}
