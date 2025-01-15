#include "../environment.h"

void env::detail::MappingAccess::CheckLoadable(const std::vector<env::BlockExport>& exports) {
	env::Instance()->mapping().fCheckLoadable(exports);
}
void env::detail::MappingAccess::BlockLoaded(const std::vector<env::BlockExport>& exports) {
	env::Instance()->mapping().fBlockExports(exports);
}
void env::detail::MappingAccess::CheckFlush() {
	env::Instance()->mapping().fCheckFlush();
}
uintptr_t env::detail::MappingAccess::Configure() {
	/* return the highest accessed address used by the mapper */
	return uintptr_t(env::Instance()->mapping().pCaches + detail::BlockCacheCount);
}
uintptr_t env::detail::MappingAccess::CacheAddress() {
	return uintptr_t(env::Instance()->mapping().pCaches);
}
