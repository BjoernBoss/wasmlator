/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025-2026 Bjoern Boss Henrichsen */
#pragma once

#include "../gen-common.h"
#include "gen-writer.h"
#include "gen-superblock.h"
#include "gen-address.h"
#include "../memory/memory-builder.h"
#include "../mapping/mapping-builder.h"
#include "../process/process-builder.h"
#include "../context/context-builder.h"
#include "../core/core-builder.h"

namespace gen {
	namespace detail {
		struct BlockState {
			uint32_t blockCallbackId = 0;
			uint32_t chunkCallbackId = 0;
			uint32_t instCallbackId = 0;
			uint32_t debugCheckCallbackId = 0;
		};

		struct BlockAccess {
			static bool Setup(detail::BlockState& state);
		};
	}

	/* env::Process instance and gen::Generator instance must be created
	*	Note: blocks must not be flushed mid block-creation
	*	Note: after closing the block, no changes can be made to the block anymore */
	class Block {
		friend struct detail::BlockAccess;
	private:
		detail::Addresses pAddresses;
		detail::MemoryState pMemory;
		detail::MappingState pMapping;
		detail::ContextState pContext;
		detail::InteractState pInteract;

	public:
		Block(wasm::Module& mod);
		Block(gen::Block&&) = delete;
		Block(const gen::Block&) = delete;

	private:
		void fProcess(const detail::OpenAddress& next);

	public:
		void run(env::guest_t address);
		std::vector<env::BlockExport> close();
	};
}
