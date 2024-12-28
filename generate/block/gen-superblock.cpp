#include "gen-superblock.h"
#include "../generate.h"

static host::Logger logger{ u8"gen::block" };

namespace I = wasm::inst;

gen::detail::SuperBlock::SuperBlock(wasm::Sink& sink, const detail::ContextState& context, env::guest_t address) : pSink{ sink }, pContext{ context, sink }, pNextAddress{ address } {}

size_t gen::detail::SuperBlock::fLookup(env::guest_t address) const {
	size_t first = 0, last = pList.size() - 1;

	while (first != last) {
		size_t index = (first + last) / 2;
		if (address > pList[index].address)
			first = index + 1;
		else
			last = index;
	}

	return (pList[first].address == address ? first : pList.size());
}
std::set<gen::detail::InstRange> gen::detail::SuperBlock::fSetupRanges() const {
	std::set<gen::detail::InstRange> out;

	/* collect all ranges in the instructions */
	for (size_t i = 0; i < pList.size(); ++i) {
		const auto& inst = pList[i];
		if (!inst.branches)
			continue;

		/* resolve the jump-target and check if it lies in the current super-block */
		size_t target = fLookup(inst.target);
		if (target == pList.size())
			continue;

		/* check if the direction is backwards (also the case for equal, as a jump to itself is considered a backwards jump) */
		bool backwards = (target <= i);

		/* construct the range as minimally as necessary (i.e. must at least wrap both the instruction at index [first] and at index [last]) */
		size_t first = std::min<size_t>(i, target);
		size_t last = std::max<size_t>(i, target);
		if (!backwards)
			--last;
		out.insert({ first, last, 0, backwards });
	}
	return out;
}
size_t gen::detail::SuperBlock::fIrreducibleConflicts(detail::RangeIt test, detail::RangeIt begin, detail::RangeIt end) const {
	size_t count = 0;

	/* iterate over the ranges and find all forward ranges, which jump into the backward-range to be tested */
	for (detail::RangeIt it = begin; it != end; ++it) {
		if (!it->backwards && it->last >= test->first && it->last < test->last)
			++count;
	}
	return count;
}
gen::detail::RangeIt gen::detail::SuperBlock::fConflictCluster(std::set<detail::InstRange>& set, detail::RangeIt begin, detail::RangeIt end, size_t first, size_t last) {
	/* the expansion-modes will also remove any duplicate ranges, which are potentially shorter, but jump
	*	to the same address - this has to be done before adding the irreducible-conflict resolving ranges,
	*	as they add logic to the header of the corresponding loops, which can therefore not be used as a
	*	replacement path for other jumps, as they must not interact with the given logic */
	enum class Mode : uint8_t {
		none,
		expandForwards,
		expandBackwards
	} mode = Mode::none;

	/* iterate over the ranges and both detect and apply the conflict resolving pattern */
	for (detail::RangeIt it = begin; it != end;) {
		bool isFirst = (it == begin);

		/* check if a forward expansion is possible (i.e. a jump forward's start index is moved up) */
		if (!it->backwards && it->last == last && (mode == Mode::expandForwards || mode == Mode::none)) {
			/* check if this is the first expansion */
			if (mode == Mode::none) {
				mode = Mode::expandForwards;
				pRanges.insert(detail::InstRange{ first, last, pTargets.size(), false });
				pTargets.push_back(Target{ {}, pList[last + 1].address, false });
			}

			/* remove the range from the conflicting cluster */
			it = set.erase(it);
			if (isFirst)
				begin = it;
			continue;
		}

		/* check if a backward expansion is possible (i.e. a jump backward's last index is moved down) */
		if (it->backwards && it->first == first && (mode == Mode::expandBackwards || mode == Mode::none)) {
			/* check if this is the first expansion */
			if (mode == Mode::none) {
				mode = Mode::expandBackwards;
				pRanges.insert(detail::InstRange{ first, last, pTargets.size(), true });
				pTargets.push_back(Target{ {}, pList[first].address, false });
			}

			/* remove the range from the conflicting cluster */
			it = set.erase(it);
			if (isFirst)
				begin = it;
			continue;
		}
		++it;
	}

	/* check if a pattern could already be applied to potentially resolve the conflict */
	if (mode != Mode::none)
		return begin;

	/* iterate over the ranges again and find candidates for the irreducible loop to be resolved
	*	(there must be one, as the other patterns would otherwise have resolved the issue)
	*
	*	Note: due to the nature of web-assembly loop constriants, irreducible control-flows can only be resolved
	*	by expanding a backward jump all the way to the top and adding an additional forward jump, because the other
	*	solution would require a block, which afterwards jumps back up to the loop-start of a nested loop... */
	detail::RangeIt irreducibleIt = end;
	size_t irreducibleCount = 0;
	for (detail::RangeIt it = begin; it != end; ++it) {
		/* check if this is a candidate for a conflict */
		if (!it->backwards || it->last != last)
			continue;
		size_t count = fIrreducibleConflicts(it, begin, end);
		if (irreducibleCount < count) {
			irreducibleIt = it;
			irreducibleCount = count;
		}
	}

	/* consistency check: There must be one backward jump ending at [last], as expansion would otherwise have been possible */
	if (irreducibleCount == 0)
		logger.fatal(u8"Inconsistency while resolving irreducible control-flow detected");

	/* insert the new overall range to the output and the intermediate range (no need to add it to the input, as
	*	it will act like an expansion, as it is a forward-jump starting at the first instruction, and can therefore
	*	not conflict with any other ranges anymore - nesting will automatically be ensured by sorted set) */
	pRanges.insert(detail::InstRange{ first, last, pTargets.size(), true });
	pRanges.insert(detail::InstRange{ first, irreducibleIt->first - 1, pTargets.size(), false });

	/* setup the target slot */
	pTargets.push_back(Target{ {}, pList[irreducibleIt->first].address, true });

	/* remove the conflicting range from the input set */
	if (irreducibleIt == begin)
		return set.erase(irreducibleIt);
	set.erase(irreducibleIt);
	return begin;
}

bool gen::detail::SuperBlock::push(const gen::Instruction& inst) {
	/* configure the new instruction-entry */
	Entry& entry = pList.emplace_back();
	entry.self = inst.self;
	entry.address = pNextAddress;
	entry.target = inst.target;
	entry.branches = (inst.type == gen::InstType::conditionalDirect || inst.type == gen::InstType::jumpDirect);
	entry.invalid = (inst.type == gen::InstType::invalid);

	/* check if its an invalid instruction, in which case the block will be ended */
	if (inst.type == gen::InstType::invalid) {
		logger.debug(u8"Unable to decode instruction [", str::As{ U"#018x", pNextAddress }, u8']');
		return false;
	}

	/* validate the extent and advance the address */
	if (inst.size == 0)
		logger.fatal(u8"Instruction at [", str::As{ U"#018x", pNextAddress }, u8"] cannot have a size of 0");
	pNextAddress += inst.size;

	/* check if the current strand can be continued or if the overall super-block is considered closed */
	if (gen::Instance()->singleStep())
		return false;
	if (inst.type != gen::InstType::endOfBlock && inst.type != gen::InstType::jumpDirect)
		return true;

	/* iterate over the instructions and check if any of the jumps jump to the current next
	*	address, in which case the block can be continued (iterate over all instructions again,
	*	as previous instructions might become valid after another strand has been processed)
	*
	*	Note: Ignore the case of a jump into an instruction, as it needs to be translated as a new super-block anyways */
	for (size_t i = 0; i < pList.size(); ++i) {
		if (!pList[i].branches)
			continue;

		/* check if the instruction jumps to the current end */
		if (pList[i].target == pNextAddress)
			return true;
	}
	return false;
}
void gen::detail::SuperBlock::setupRanges() {
	/* check if this is single-step mode, in which case the iterators can just be reset */
	if (gen::Instance()->singleStep()) {
		pIndex = 0;
		pIt = pRanges.begin();
		return;
	}

	/* create the set of jump-ranges */
	std::set<detail::InstRange> raw = fSetupRanges();

	/* iterate over the ranges and find all conflicting clusters, fix them, and write them out to the final ranges */
	for (detail::RangeIt it = raw.begin(); it != raw.end();) {
		size_t last = it->last;
		bool multi = false;

		/* advance the iterator to the first non-overlapping range */
		detail::RangeIt nt = it;
		for (++nt; nt != raw.end() && last >= nt->first; ++nt) {
			last = std::max(last, nt->last);
			multi = true;
		}

		/* check if multiple conflicting ranges have been found and fix them
		*	one by one and otherwise fush out the single range immediately */
		if (multi)
			nt = fConflictCluster(raw, it, nt, it->first, last);
		else {
			pRanges.insert(detail::InstRange{ it->first, it->last, pTargets.size(), it->backwards });
			pTargets.push_back(Target{ {}, pList[it->backwards ? it->first : it->last + 1].address, false });
		}
		it = nt;
	}

	/* reset the iterator and counter */
	pIndex = 0;
	pIt = pRanges.begin();
}
env::guest_t gen::detail::SuperBlock::nextFetch() const {
	return pNextAddress;
}

gen::detail::InstTarget gen::detail::SuperBlock::lookup(env::guest_t target) const {
	/* linear lookup as stack should never become really large */
	for (size_t i = 0; i < pStack.size(); ++i) {
		if (pStack[i].address == target && !pStack[i].internal)
			return { &pStack[i].target, true };
	}
	return { 0, false };
}
bool gen::detail::SuperBlock::next() {
	pChunk.clear();

	/* check if the end has been reached (either because only one remaining un-decodable instruction has been encountered or because
	*	all chunks have been processed; block cannot be empty, as at least one push will occur before any chunks are fetched) */
	bool lastAndInvalid = (pIndex < pList.size() && pList[pIndex].invalid);
	if (lastAndInvalid || pIndex >= pList.size()) {
		pStack.clear();

		/* check if the not-decodable stub needs to be added */
		if (lastAndInvalid) {
			pContext.makeNotDecodable(pNextAddress);
			pSink[I::Unreachable()];
		}

		/* add the single-step-handler */
		else if (gen::Instance()->singleStep())
			pSink[I::U64::Const(pNextAddress)];

		/* add the not-reachable stub, as this address should never be reached as the super-block would otherwise have been continued */
		else {
			pContext.makeNotReachable(pNextAddress);
			pSink[I::Unreachable()];
		}
		return false;
	}

	/* pop all closed ranges */
	while (!pStack.empty() && pIndex > pStack.back().last)
		pStack.pop_back();

	/* check if the next ranges need to be opened */
	while (pIt != pRanges.end() && pIndex == pIt->first) {
		Target& target = pTargets[pIt->target];

		/* check if this is a header-range of a resolved irreducible conflict */
		if (target.irreducible && pIt->backwards) {
			/* allocate the state variable */
			if (!target.cfg.valid())
				target.cfg = pSink.local(wasm::Type::i32, str::u8::Build(u8"_cfg_", pIt->target));

			/* add the code to reset the variable upon entry of the loop backwards */
			pSink[I::U32::Const(0)];
			pSink[I::Local::Set(target.cfg)];
		}

		/* add the next header (mark the jump as internal and thereby not accessible,
		*	if its the forward-jump of a resolved irreducible control-flow) */
		if (pIt->backwards)
			pStack.push_back({ wasm::Loop{ pSink }, target.address, pIt->last, false });
		else
			pStack.push_back({ wasm::Block{ pSink }, target.address, pIt->last, target.irreducible });

		/* check if the conditional forward-jump of the resolved irreducible control-flow needs to be added */
		if (!pIt->backwards && target.irreducible) {
			/* check if the corresponding loop was entered from its neighbors, not by jumping */
			pSink[I::Local::Get(target.cfg)];
			pSink[I::U32::EqualZero()];
			wasm::IfThen _if{ pSink };

			/* mark the loop as now being entered in a valid fashion (no need to ensure this is added
			*	immediately after the loop-header, as they will both have the same first-address, and
			*	therefore it will be executed before any translator-generated code will be executed) */
			pSink[I::U32::Const(1)];
			pSink[I::Local::Set(target.cfg)];

			/* add the immediate jump to the actual target address */
			_if.otherwise();
			pSink[I::Branch::Direct(pStack.back().target)];
		}
		++pIt;
	}

	/* extract the number of instructions until the next range either needs to start or has to be closed */
	size_t last = pList.size() - 1;
	if (!pStack.empty() && pStack.back().last < last)
		last = pStack.back().last;
	if (pIt != pRanges.end() && pIt->first <= last)
		last = pIt->first - 1;

	/* check if the chunk contains a potential last un-decodable instruction, and remove it from this chunk (cannot
	*	be the only instruction, as the header-guard of this function would otherwise already have caught it) */
	if (last + 1 == pList.size() && pList.back().invalid)
		--last;

	/* setup the start-address of the chunk and write the chunk-self values out */
	pCurrentChunk = pList[pIndex].address;
	for (; pIndex < last + 1; ++pIndex)
		pChunk.push_back(pList[pIndex].self);
	return true;
}
const std::vector<uintptr_t>& gen::detail::SuperBlock::chunk() const {
	return pChunk;
}
env::guest_t gen::detail::SuperBlock::chunkStart() const {
	return pCurrentChunk;
}
