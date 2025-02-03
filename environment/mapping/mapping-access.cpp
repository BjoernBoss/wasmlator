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
