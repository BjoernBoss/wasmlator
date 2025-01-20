#include "util-unique.h"

namespace global {
	static uint64_t NextId = 0;
}

uint64_t util::UniqueId() {
	return ++global::NextId;
}
