#include "../environment.h"

void env::detail::FileSystemAccess::CleanupFiles() {
	return env::Instance()->filesystem().fCloseAll();
}
