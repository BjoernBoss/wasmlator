#pragma once

#include "../gen-common.h"

namespace gen {
	/* No requirements regarding construction of env::Process */
	void SetupGlue(wasm::Module& mod);
}
