#pragma once

#include "../trans-common.h"

namespace trans {
	/* No requirements regarding construction of env::Process */
	void SetupGlue(wasm::Module& mod);
}
