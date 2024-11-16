#pragma once

#include "../gen-common.h"
#include "../translator/gen-writer.h"
#include "../translator/gen-superblock.h"
#include "../memory/memory-builder.h"
#include "../mapping/mapping-builder.h"
#include "../context/context-builder.h"
#include "../address/gen-address.h"

namespace gen {
	/* env::Process instance must be created
	*	Note: blocks must not be flushed mid block-creation */
	class Translator {
	private:
		detail::Addresses pAddresses;
		detail::MemoryState pMemory;
		detail::MappingState pMapping;
		detail::ContextState pContext;
		detail::InteractState pInteract;

	public:
		Translator(wasm::Module& mod);
		Translator(gen::Translator&&) = delete;
		Translator(const gen::Translator&) = delete;

	private:
		void fProcess(const detail::OpenAddress& next);

	public:
		void run(env::guest_t address);
		std::vector<env::BlockExport> close();
	};
}
