#pragma once

#include "trans-common.h"
#include "trans-writer.h"
#include "memory/memory-builder.h"
#include "mapping/mapping-builder.h"
#include "context/context-builder.h"
#include "address/trans-address.h"

namespace trans {
	struct TranslationInterface {
		virtual trans::Instruction fetch(env::guest_t addr) = 0;
		virtual void produce(const trans::Writer& writer, const trans::Instruction* data, size_t count) = 0;
	};

	class Translator {
	private:
		struct Range {
		public:
			size_t first = 0;
			size_t last = 0;
			bool backwards = false;

		public:
			friend bool operator<(const Range& l, const Range& r) {
				return (l.first < r.first || (l.first == r.first && l.last < r.last));
			}
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
		size_t fLookup(env::guest_t address, const std::vector<trans::Instruction>& list) const;
		void fFetchSuperBlock(env::guest_t address, std::vector<trans::Instruction>& list) const;
		void fSetupRanges(const std::vector<trans::Instruction>& list, std::set<Range>& ranges) const;

	private:
		void fProcess(const detail::OpenAddress& next);

	public:
		void run(env::guest_t address);
		std::vector<env::BlockExport> close();
	};
}
