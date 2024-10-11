#pragma once

#include "../env-common.h"

namespace env {
	namespace bridge {
		struct Blocks;
	}

	class Blocks {
		friend struct bridge::Blocks;
	private:
		struct BlockCache {
			env::guest_t address = 0;
			uint32_t index = 0;
		};

	private:
		env::Process* pProcess = 0;
		uint32_t pCacheAddress = 0;

	public:
		Blocks(env::Process* process);
		Blocks(env::Blocks&&) = delete;
		Blocks(const env::Blocks&) = delete;

	private:
		uint32_t fLookup(env::guest_t address) const;

	public:
		void setupCoreImports(wasm::Module& mod, env::CoreState& state);
		void setupCoreBody(wasm::Module& mod, env::CoreState& state) const;
		void setupBlockImports(wasm::Module& mod, env::BlockState& state) const;

	public:
		void flushBlocks();
	};
}
