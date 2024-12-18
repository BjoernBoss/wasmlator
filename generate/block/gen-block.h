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
	/* env::Process instance must be created
	*	Note: blocks must not be flushed mid block-creation */
	class Block {
	private:
		gen::Translator* pTranslator = 0;
		detail::Addresses pAddresses;
		detail::MemoryState pMemory;
		detail::MappingState pMapping;
		detail::ProcessState pProcess;
		detail::ContextState pContext;
		detail::InteractState pInteract;

	public:
		Block(wasm::Module& mod, gen::Translator* translator);
		Block(gen::Block&&) = delete;
		Block(const gen::Block&) = delete;

	private:
		void fProcess(const detail::OpenAddress& next);

	public:
		void run(env::guest_t address);
		std::vector<env::BlockExport> close();
	};
}
