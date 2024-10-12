#pragma once

#include "../env-common.h"

namespace env::detail {
	class ContextBuilder {
	private:
		env::Process* pProcess = 0;

	public:
		ContextBuilder(env::Process* process);

	public:
		void setupCoreImports(wasm::Module& mod, env::CoreState& state);
	};
}
