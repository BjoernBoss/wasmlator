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
			size_t origin = 0;
			bool backwards = false;

		public:
			friend bool operator<(const Range& l, const Range& r) {
				if (l.first != r.first)
					return (l.first < r.first);
				if (l.last != r.last)
					return (l.last < r.last);
				if (l.origin != r.origin)
					return (l.origin < r.origin);
				return (!l.backwards && r.backwards);
			}
		};

	private:
		detail::Addresses pAddresses;
		detail::MemoryState pMemory;
		detail::MappingState pMapping;
		detail::ContextState pContext;
		trans::TranslationInterface* pInterface = 0;

	public:
		Translator(bool core, wasm::Module& mod, trans::TranslationInterface* interface, size_t maxDepth);
		Translator(trans::Translator&&) = delete;
		Translator(const trans::Translator&) = delete;

	private:
		void fSetupForCore(wasm::Module& mod);
		void fSetupForBlock(wasm::Module& mod);

	private:
		size_t fLookup(env::guest_t address, const std::vector<trans::Instruction>& list) const;
		void fFetchSuperBlock(env::guest_t address, std::vector<trans::Instruction>& list) const;
		void fSetupRanges(const std::vector<trans::Instruction>& list, std::set<Range>& ranges) const;
		bool fExpandRanges(std::set<Range>& ranges) const;
		void fFixRanges(std::set<Range>& ranges) const;

	private:
		void fProcess(const detail::OpenAddress& next);

	public:
		void run(env::guest_t address);
		std::vector<env::BlockExport> close();
	};
}
