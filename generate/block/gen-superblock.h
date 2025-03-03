/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025 Bjoern Boss Henrichsen */
#pragma once

#include "../gen-common.h"
#include "../context/context-writer.h"

namespace gen::detail {
	enum class RangeType : uint8_t {
		backwards,
		conditional,
		forwards
	};
	struct InstRange {
	public:
		size_t first = 0;
		size_t last = 0;
		detail::RangeType type = detail::RangeType::backwards;

	public:
		/* must be sorted such that for two ranges with the same start, the larger one comes
		*	first, and for identical ranges, backwards < conditional < forwards */
		friend bool operator<(const detail::InstRange& l, const detail::InstRange& r) {
			if (l.first != r.first)
				return (l.first < r.first);
			if (l.last != r.last)
				return (l.last > r.last);
			return (l.type < r.type);
		}
	};
	struct InstTarget {
		const wasm::Target* target = 0;
		size_t index = 0;
		bool conditional = false;
	};

	using RangeIt = std::set<detail::InstRange>::iterator;

	class SuperBlock {
	private:
		struct Stack {
			wasm::Target target;
			size_t first = 0;
			size_t last = 0;
			detail::RangeType type = detail::RangeType::backwards;
		};
		struct Target {
			env::guest_t address = 0;
			size_t stack = 0;
			size_t first = 0;
			size_t index = 0;
			bool conditional = false;
			bool active = false;
		};
		struct Entry {
			uintptr_t self = 0;
			env::guest_t address = 0;
			env::guest_t target = 0;
			bool branches = false;
			bool invalid = false;
			bool readFailure = false;
		};

	private:
		std::vector<Stack> pStack;
		std::vector<Target> pTargets;
		std::vector<Entry> pList;
		std::vector<uintptr_t> pChunk;
		std::set<detail::InstRange> pRanges;
		detail::ContextWriter pContext;
		detail::RangeIt pIt;
		env::guest_t pNextAddress = 0;
		env::guest_t pCurrentChunk = 0;
		size_t pIndex = 0;

	public:
		SuperBlock(const detail::ContextState& context, env::guest_t address);

	private:
		size_t fLookup(env::guest_t address) const;
		std::set<detail::InstRange> fSetupRanges() const;
		void fConflictCluster(std::set<detail::InstRange>& set, detail::RangeIt begin, detail::RangeIt end, size_t first, size_t last);

	private:
		void fPrepareStack();
		void fPrepareChunk(bool single);

	public:
		bool push(const gen::Instruction& inst);
		void readFailure();
		void setupRanges();
		env::guest_t nextFetch() const;

	public:
		detail::InstTarget lookup(env::guest_t target) const;
		bool next(bool single);
		void finalize();
		const std::vector<uintptr_t>& chunk() const;
		env::guest_t chunkStart() const;
	};
}
