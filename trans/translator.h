#pragma once

#include <queue>
#include <unordered_map>
#include <vector>

#include "trans-common.h"
#include "memory/memory-builder.h"
#include "mapping/mapping-builder.h"

namespace trans {
	struct TranslationInterface {
		virtual trans::Instruction fetch(env::guest_t addr) = 0;
	};

	class Translator {
	private:
		struct Entry {
			env::guest_t address = 0;
			size_t depth = 0;
		};
		struct Translated {
			trans::Instruction inst;
			env::guest_t address = 0;
			size_t index = 0;
			bool local = false;
			bool startOfBlock = false;
		};

	private:
		wasm::Module& pModule;
		detail::MemoryState pMemory;
		detail::MappingState pMapping;

		std::unordered_map<env::guest_t, wasm::Function> pTranslated;
		std::queue<Entry> pQueue;
		env::Mapping* pMapping_ = 0;
		trans::TranslationInterface* pInterface = 0;
		size_t pMaxDepth = 0;

	private:
		Translator(wasm::Module& mod, env::Mapping* mapping, trans::TranslationInterface* interface, size_t maxDepth);

	public:
		static trans::Translator CoreModule(wasm::Module& mod, env::Process* process, trans::TranslationInterface* interface, size_t maxDepth);
		static trans::Translator BlockModule(wasm::Module& mod, env::Process* process, trans::TranslationInterface* interface, size_t maxDepth);

	private:
		size_t fLookup(env::guest_t address, const std::vector<Translated>& list) const;
		void fFetchSuperBlock(env::guest_t address, std::vector<Translated>& list);

	private:
		void fPush(env::guest_t address, size_t depth);
		void fProcess(env::guest_t address, size_t depth);

	public:
		void run(env::guest_t address);
		void close();
	};
}
