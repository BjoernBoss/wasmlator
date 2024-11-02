#pragma once

#include <wasgen/wasm.h>
#include <ustring/ustring.h>
#include <queue>
#include <unordered_map>
#include <vector>

#include "../env/env-process.h"

namespace trans {
	enum class InstType : uint8_t {
		primitive,
		jumpDirect,
		jumpIndirect,
		conditionalDirect,
		call,
		endOfBlock,
		invalid
	};

	struct TranslationException : public str::BuildException {
		template <class... Args>
		constexpr TranslationException(const Args&... args) : str::BuildException{ args... } {}
	};

	struct Instruction {
		uintptr_t data = 0;
		env::guest_t target = 0;
		size_t size = 0;
		trans::InstType type = trans::InstType::primitive;
	};

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
		std::unordered_map<env::guest_t, wasm::Function> pTranslated;
		std::queue<Entry> pQueue;
		env::Mapping* pMapping = 0;
		trans::TranslationInterface* pInterface = 0;
		size_t pMaxDepth = 0;

	public:
		Translator(wasm::Module& mod, env::Mapping* mapping, trans::TranslationInterface* interface, size_t maxDepth);

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
