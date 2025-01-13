#pragma once

#include "../env-common.h"

namespace env::detail {
	struct FileSystemAccess {
		static void CleanupFiles();
	};
}
