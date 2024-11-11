#pragma once

#include "../env-common.h"

namespace env::detail {
	struct InteractBridge {
		/* exports */
		static void InvokeVoid(uint32_t index);
		static uint64_t InvokeParam(uint64_t param, uint32_t index);

		/* imports */
		static void CallVoid(uint32_t index);
		static uint64_t CallParam(uint64_t param, uint32_t index);
	};
}
