#pragma once

#include "trans-common.h"
#include "translator/trans-writer.h"
#include "translator/trans-superblock.h"
#include "memory/memory-builder.h"
#include "mapping/mapping-builder.h"
#include "context/context-builder.h"
#include "address/trans-address.h"

namespace trans {
	struct TranslationInterface {
		virtual void blockStarted() = 0;
		virtual void blockCompleted() = 0;
		virtual trans::Instruction fetch(env::guest_t addr) = 0;
		virtual void produce(const trans::Writer& writer, const trans::Instruction* data, size_t count) = 0;
	};

	class Translator {
	private:
		detail::Addresses pAddresses;
		detail::MemoryState pMemory;
		detail::MappingState pMapping;
		detail::ContextState pContext;
		trans::TranslationInterface* pInterface = 0;

	public:
		Translator(wasm::Module& mod, trans::TranslationInterface* interface, size_t maxDepth);
		Translator(trans::Translator&&) = delete;
		Translator(const trans::Translator&) = delete;

	private:
		void fProcess(const detail::OpenAddress& next);

	public:
		void run(env::guest_t address);
		std::vector<env::BlockExport> close();
	};
}
