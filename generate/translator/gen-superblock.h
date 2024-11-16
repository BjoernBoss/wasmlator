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
		size_t target = 0;
		bool backwards = false;

	public:
		/* must be sorted such that for two ranges with the same start, the larger one comes first */
		friend bool operator<(const detail::InstRange& l, const detail::InstRange& r) {
			if (l.first != r.first)
				return (l.first < r.first);
			if (l.last != r.last)
				return (l.last > r.last);
			if (l.target != r.target)
				return (l.target < r.target);
			return (!l.backwards && r.backwards);
		}
	};
	using RangeIt = std::set<detail::InstRange>::iterator;

	class SuperBlock {
	private:
		struct Stack {
			wasm::Target target;
			env::guest_t address = 0;
			size_t last = 0;
			bool internal = false;
		};
		struct Target {
			wasm::Variable cfg;
			env::guest_t address = 0;
			bool irreducible = false;
		};

	private:
		std::vector<Stack> pStack;
		std::vector<Target> pTargets;
		std::vector<gen::Instruction> pList;
		std::set<detail::InstRange> pRanges;
		detail::RangeIt pIt;
		size_t pIndex = 0;

	public:
		SuperBlock() = default;

	private:
		size_t fLookup(env::guest_t address) const;
		std::set<detail::InstRange> fSetupRanges() const;
		size_t fIrreducibleConflicts(detail::RangeIt test, detail::RangeIt begin, detail::RangeIt end) const;
		detail::RangeIt fConflictCluster(std::set<detail::InstRange>& set, detail::RangeIt begin, detail::RangeIt end, size_t first, size_t last);

	public:
		bool push(const gen::Instruction& inst);
		bool incomplete(env::guest_t address) const;
		void setupRanges();

	public:
		const wasm::Target* lookup(const gen::Instruction& inst) const;
		detail::InstChunk next(wasm::Sink& sink);
	};
}
