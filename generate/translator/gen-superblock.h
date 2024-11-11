#pragma once

#include "../gen-common.h"

namespace gen::detail {
	struct InstChunk {
		const gen::Instruction* inst = 0;
		size_t count = 0;
	};
	struct InstRange {
	public:
		size_t first = 0;
		size_t last = 0;
		size_t origin = 0;
		bool backwards = false;

	public:
		friend bool operator<(const detail::InstRange& l, const detail::InstRange& r) {
			if (l.first != r.first)
				return (l.first < r.first);
			if (l.last != r.last)
				return (l.last < r.last);
			if (l.origin != r.origin)
				return (l.origin < r.origin);
			return (!l.backwards && r.backwards);
		}
	};

	class SuperBlock {
	private:
		struct Stack {
			wasm::Target target;
			env::guest_t address = 0;
			size_t last = 0;
		};

	private:
		std::vector<Stack> pStack;
		std::vector<gen::Instruction> pList;
		std::set<detail::InstRange> pRanges;
		std::set<detail::InstRange>::iterator pIt;
		size_t pIndex = 0;

	public:
		SuperBlock() = default;

	private:
		size_t fLookup(env::guest_t address) const;
		void fSetupRanges();
		bool fExpandRanges();
		void fFixRanges();

	public:
		bool push(const gen::Instruction& inst);
		bool incomplete(env::guest_t address) const;
		void setupRanges();

	public:
		const wasm::Target* lookup(env::guest_t address) const;
		detail::InstChunk next(wasm::Sink& sink);
	};
}
