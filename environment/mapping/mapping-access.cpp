#include "../env-process.h"

bool env::detail::MappingAccess::CheckLoadable(const std::vector<env::BlockExport>& exports) {
	return env::Instance()->mapping().fCheckLoadable(exports);
}
void env::detail::MappingAccess::BlockLoaded(const std::vector<env::BlockExport>& exports) {
	env::Instance()->mapping().fBlockExports(exports);
}
uintptr_t env::detail::MappingAccess::Configure() {
	/* return the highest accessed address used by the mapper */
	return uintptr_t(env::Instance()->mapping().pCaches + detail::BlockCacheCount);
}
uintptr_t env::detail::MappingAccess::CacheAddress() {
	return uintptr_t(env::Instance()->mapping().pCaches);
}
