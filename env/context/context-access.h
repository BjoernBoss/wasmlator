#pragma once

#include "../env-common.h"

namespace env::detail {
	class ContextAccess {
	private:
		env::Process* pProcess = 0;

	public:
		ContextAccess(env::Process* process);

	public:
		bool create(std::function<void(env::guest_t)> translate);
	};

	class ContextBuilder {
	private:
		const env::Process* pProcess = 0;

	public:
		ContextBuilder(const env::Process* process);

	public:
		void setupCoreImports(wasm::Module& mod, env::CoreState& state) const;
	};
}
