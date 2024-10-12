#pragma once

#include "../env-common.h"
#include "blocks-builder.h"
#include "blocks-bridge.h"

namespace env {
	class Blocks {
		friend struct bridge::Blocks;
		friend class detail::BlocksBuilder;
	private:
		struct BlockCache {
			env::guest_t address = 0;
			uint32_t index = 0;
		};

	private:
		std::unordered_map<env::guest_t, uint32_t> pMapping;
		env::Process* pProcess = 0;
		uint32_t pCacheAddress = 0;

	public:
		Blocks(env::Process* process);
		Blocks(env::Blocks&&) = delete;
		Blocks(const env::Blocks&) = delete;

	private:
		uint32_t fLookup(env::guest_t address) const;
		void fAssociate(env::guest_t address, uint32_t index);
		void fFlushed();

	public:
		void makeGoto(const wasm::Variable& i64Address, const env::ModuleState& state) const;
		void makeLookup(const wasm::Variable& i64Address, const env::ModuleState& state) const;

	public:
		void execute(env::guest_t address);
		void flushBlocks();
	};
}
