#pragma once

#include "../gen-common.h"
#include "../process/process-writer.h"

namespace gen::detail {
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
	struct InstTarget {
		const wasm::Target* target = 0;
		bool directJump = false;
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
		struct Entry {
			uintptr_t self = 0;
			env::guest_t address = 0;
			env::guest_t target = 0;
			bool branches = false;
			bool invalid = false;
		};

	private:
		std::vector<Stack> pStack;
		std::vector<Target> pTargets;
		std::vector<Entry> pList;
		std::vector<uintptr_t> pChunk;
		std::set<detail::InstRange> pRanges;
		wasm::Sink& pSink;
		detail::ProcessWriter pProcess;
		detail::RangeIt pIt;
		env::guest_t pNextAddress = 0;
		env::guest_t pCurrentChunk = 0;
		size_t pIndex = 0;

	public:
		SuperBlock(wasm::Sink& sink, const detail::ProcessState& process, env::guest_t address);

	private:
		size_t fLookup(env::guest_t address) const;
		std::set<detail::InstRange> fSetupRanges() const;
		size_t fIrreducibleConflicts(detail::RangeIt test, detail::RangeIt begin, detail::RangeIt end) const;
		detail::RangeIt fConflictCluster(std::set<detail::InstRange>& set, detail::RangeIt begin, detail::RangeIt end, size_t first, size_t last);

	public:
		bool push(const gen::Instruction& inst);
		void setupRanges();
		env::guest_t nextFetch() const;

	public:
		detail::InstTarget lookup(env::guest_t target) const;
		bool next();
		const std::vector<uintptr_t>& chunk() const;
		env::guest_t chunkStart() const;
	};
}
