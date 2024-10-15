#pragma once

#include "../env-common.h"

namespace env::detail {
	class ContextAccess {
	private:
		env::Process* pProcess = 0;

	public:
		ContextAccess(env::Process* process);

	public:
		bool create(std::function<void(env::guest_t)> translate, std::function<void(int32_t)> terminated);
	};

	class ContextBuilder {
	private:
		const env::Process* pProcess = 0;

	public:
		ContextBuilder(const env::Process* process);

	public:
		void setupCoreImports(wasm::Module& mod, env::CoreState& state) const;
		void setupCoreBody(wasm::Module& mod, env::CoreState& state) const;
		void setupBlockImports(wasm::Module& mod, env::ModuleState& state) const;
	};
}
