#pragma once

#include "../env-common.h"

namespace env::detail {
	class ContextInteract {
	private:
		env::Process* pProcess = 0;

	public:
		ContextInteract(env::Process* process);

	public:
		bool create(std::function<void(env::guest_t)> translate);
		bool setCore(const uint8_t* data, size_t size, std::function<void(bool)> callback);
	};

	class ContextBuilder {
	private:
		env::Process* pProcess = 0;

	public:
		ContextBuilder(env::Process* process);

	public:
		void setupCoreImports(wasm::Module& mod, env::CoreState& state);
	};
}
