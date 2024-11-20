#pragma once

#include "../env-common.h"

namespace env::detail {
	struct ContextAccess {
		static uintptr_t Configure();
		static uintptr_t ContextAddress();
		static size_t ContextSize();
	};
}
