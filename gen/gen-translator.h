#pragma once

#include "gen-common.h"
#include "translator/gen-writer.h"
#include "translator/gen-superblock.h"
#include "memory/memory-builder.h"
#include "mapping/mapping-builder.h"
#include "context/context-builder.h"
#include "address/gen-address.h"

namespace gen {
	struct TranslationInterface {
		virtual void blockStarted() = 0;
		virtual void blockCompleted() = 0;
		virtual gen::Instruction fetch(env::guest_t address) = 0;
		virtual void produce(const gen::Writer& writer, const gen::Instruction* data, size_t count) = 0;
	};

	class Translator {
	private:
		detail::Addresses pAddresses;
		detail::MemoryState pMemory;
		detail::MappingState pMapping;
		detail::ContextState pContext;
		gen::TranslationInterface* pInterface = 0;

	public:
		Translator(wasm::Module& mod, gen::TranslationInterface* interface, size_t maxDepth);
		Translator(gen::Translator&&) = delete;
		Translator(const gen::Translator&) = delete;

	private:
		void fProcess(const detail::OpenAddress& next);

	public:
		void run(env::guest_t address);
		std::vector<env::BlockExport> close();
	};
}
