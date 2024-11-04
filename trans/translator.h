#pragma once

#include <queue>
#include <unordered_map>
#include <vector>

#include "trans-common.h"
#include "memory/memory-builder.h"
#include "mapping/mapping-builder.h"
#include "context/context-builder.h"
#include "address/trans-address.h"

namespace trans {
	struct TranslationInterface {
		virtual trans::Instruction fetch(env::guest_t addr) = 0;
	};

	class Translator {
	private:
		struct Translated {
			trans::Instruction inst;
			env::guest_t address = 0;
			size_t index = 0;
			bool local = false;
			bool startOfBlock = false;
		};

	private:
		detail::Addresses pAddresses;
		detail::MemoryState pMemory;
		detail::MappingState pMapping;
		detail::ContextState pContext;
		trans::TranslationInterface* pInterface = 0;

	private:
		Translator(wasm::Module& mod, trans::TranslationInterface* interface, size_t maxDepth);

	public:
		static trans::Translator CoreModule(wasm::Module& mod, trans::TranslationInterface* interface, size_t maxDepth);
		static trans::Translator BlockModule(wasm::Module& mod, trans::TranslationInterface* interface, size_t maxDepth);

	private:
		size_t fLookup(env::guest_t address, const std::vector<Translated>& list) const;
		void fFetchSuperBlock(env::guest_t address, std::vector<Translated>& list);

	private:
		void fProcess(const detail::OpenAddress& next);

	public:
		void run(env::guest_t address);
		std::vector<env::BlockExport> close();
	};
}
