#include "../generate.h"

static util::Logger logger{ u8"gen::block" };

gen::detail::SuperBlock::SuperBlock(const detail::ContextState& context, env::guest_t address) : pContext{ context }, pNextAddress{ address } {}

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
		out.insert({ first, last, (backwards ? detail::RangeType::backwards : detail::RangeType::forwards) });
	}
	return out;
}
size_t gen::detail::SuperBlock::fIrreducibleConflicts(detail::RangeIt test, detail::RangeIt begin, detail::RangeIt end) const {
	size_t count = 0;

	/* iterate over the ranges and find all forward ranges, which jump into the backward-range to be tested */
	for (detail::RangeIt it = begin; it != end; ++it) {
		if (it->type != detail::RangeType::backwards && it->last >= test->first && it->last < test->last)
			++count;
	}
	return count;
}
void gen::detail::SuperBlock::fConflictCluster(std::set<detail::InstRange>& set, detail::RangeIt begin, detail::RangeIt end, size_t first, size_t last) {
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
		/* check if a forward expansion is possible (i.e. a jump forward's start index is moved up)
		*	Note: due to the nature of conditional ranges, they will never be expanded, as they will already start at the top */
		if (it->type != detail::RangeType::backwards && it->last == last && (mode == Mode::expandForwards || mode == Mode::none)) {
			/* check if this is the first expansion */
			if (mode == Mode::none) {
				mode = Mode::expandForwards;
				pRanges.insert(detail::InstRange{ first, last, it->type });
			}

			/* remove the range from the conflicting cluster */
			it = set.erase(it);
			continue;
		}

		/* check if a backward expansion is possible (i.e. a jump backward's last index is moved down) */
		if (it->type == detail::RangeType::backwards && it->first == first && (mode == Mode::expandBackwards || mode == Mode::none)) {
			/* check if this is the first expansion */
			if (mode == Mode::none) {
				mode = Mode::expandBackwards;
				pRanges.insert(detail::InstRange{ first, last, detail::RangeType::backwards });
			}

			/* remove the range from the conflicting cluster */
			it = set.erase(it);
			continue;
		}
		++it;
	}

	/* check if a pattern could already be applied to potentially resolve the conflict */
	if (mode != Mode::none)
		return;

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
		if (it->type != detail::RangeType::backwards || it->last != last)
			continue;
		size_t count = fIrreducibleConflicts(it, begin, end);
		if (irreducibleCount < count) {
			irreducibleIt = it;
			irreducibleCount = count;
		}
	}

	/* insert the new separate ranges to the input and remove the resolved range (adding it
	*	back to the input will ensure that other ranges can take advantage of the same targets) */
	size_t target = irreducibleIt->first - 1;
	set.erase(irreducibleIt);
	set.insert(detail::InstRange{ first, last, detail::RangeType::backwards });
	set.insert(detail::InstRange{ first, target, detail::RangeType::conditional });
}

void gen::detail::SuperBlock::fFinalizeBlock(bool lastAndInvalid) {
	pStack.clear();

	/* check if the not-decodable/not-readable stub needs to be added */
	if (lastAndInvalid) {
		if (pList[pIndex].readFailure)
			pContext.makeNotReadable(pNextAddress);
		else
			pContext.makeNotDecodable(pNextAddress);
		gen::Add[I::Unreachable()];
	}

	/* add the single-step-handler */
	else if (gen::Instance()->singleStep())
		gen::Add[I::U64::Const(pNextAddress)];

	/* add the not-reachable stub, as this address should never be reached as the super-block would otherwise have been continued */
	else {
		pContext.makeNotReachable(pNextAddress);
		gen::Add[I::Unreachable()];
	}
}
void gen::detail::SuperBlock::fPrepareStack() {
	/* pop all closed ranges */
	while (!pStack.empty() && pIndex > pStack.back().last)
		pStack.pop_back();

	/* patch the remaining conditional targets, which might still be reachable
	*	via other existing backward jumps, and remove all other dead targets */
	size_t nextTarget = 0;
	for (size_t i = pTargets.size(); i > 0; --i) {
		if (!pTargets[i - 1].active) {
			nextTarget = i - 1;
			continue;
		}
		if (pTargets[i - 1].stack < pStack.size())
			continue;

		/* check if the target is not conditional, in which case it cannot be used via another backwards-jump */
		if (!pTargets[i - 1].conditional) {
			if (i == pTargets.size())
				pTargets.pop_back();
			else
				pTargets[nextTarget = (i - 1)].active = false;
			continue;
		}

		/* look for another backwards-jump on the stack, which has the same starting-index (iterate backwards to fetch the topmost value) */
		for (size_t j = pStack.size(); j > 0; --j) {
			if (pStack[j - 1].type == detail::RangeType::conditional)
				++pTargets[i - 1].preNulls;
			else if (pStack[j - 1].type == detail::RangeType::backwards && pStack[j - 1].first == pTargets[i - 1].first) {
				pTargets[i - 1].stack = j - 1;
				break;
			}
		}

		/* check if a replacement has been found and otherwise deactivate the target */
		if (pTargets[i - 1].stack >= pStack.size()) {
			if (i == pTargets.size())
				pTargets.pop_back();
			else
				pTargets[nextTarget = (i - 1)].active = false;
		}
	}

	/* helper function to get a new target */
	auto allocTarget = [&]() -> Target& {
		while (nextTarget < pTargets.size() && pTargets[nextTarget].active)
			++nextTarget;
		Target& target = (nextTarget == pTargets.size() ? pTargets.emplace_back() : pTargets[nextTarget]);
		++nextTarget;
		return target;
		};

	/* count the number of conditional ranges, which start at this index, as these many initial null-values need to be pushed to the stack */
	std::vector<wasm::Type> types;
	auto it = pIt;
	while (it != pRanges.end() && pIndex == it->first) {
		if (it->type == detail::RangeType::conditional) {
			gen::Add[I::U32::Const(0)];
			types.push_back(wasm::Type::i32);
		}
		++it;
	}

	/* check if the next ranges need to be opened */
	size_t lastBackward = pStack.size(), backwardConditionals = 0;
	while (pIt != pRanges.end() && pIndex == pIt->first) {
		/* create the next wasm-target object and setup the last-backward jump, which qualifies for
		*	jump-target for all upcoming conditionals, or patch the number of seen backward conditionals */
		if (pIt->type == detail::RangeType::backwards) {
			lastBackward = pStack.size();
			backwardConditionals = 0;
			pStack.push_back({ wasm::Loop{ gen::Sink, u8"", types, {} }, pIt->first, pIt->last, pIt->type});
		}
		else
			pStack.push_back({ wasm::Block{ gen::Sink, u8"", types, {} }, pIt->first, pIt->last, pIt->type });

		/* populate the target attributes with the first unconditional primitive entry */
		Target& target = allocTarget();
		target.address = pList[pIt->type == detail::RangeType::backwards ? pIt->first : pIt->last + 1].address;
		target.stack = pStack.size() - 1;
		target.first = pIt->first;
		target.conditional = false;
		target.active = true;
		if (pIt->type != detail::RangeType::backwards)
			target.preNulls = (target.postNulls = 0);
		else {
			target.preNulls = backwardConditionals;
			target.postNulls = types.size();
		}

		/* check if the conditional entry needs to be added as well */
		if (pIt->type != detail::RangeType::conditional) {
			++pIt;
			continue;
		}
		types.pop_back();

		/* setup the conditional entry */
		Target& cond = allocTarget();
		cond.address = target.address;
		cond.stack = lastBackward;
		cond.first = target.first;
		cond.preNulls = backwardConditionals++;
		cond.postNulls = types.size();
		cond.conditional = true;
		cond.active = true;
		++pIt;

		/* add the condition to evaluate the top-most value */
		gen::Add[I::Branch::If(pStack.back().target)];
	}
}
void gen::detail::SuperBlock::fPrepareChunk() {
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
}

bool gen::detail::SuperBlock::push(const gen::Instruction& inst) {
	/* configure the new instruction-entry */
	Entry& entry = pList.emplace_back();
	entry.self = inst.self;
	entry.address = pNextAddress;
	entry.target = inst.target;
	entry.branches = (inst.type == gen::InstType::conditionalDirect || inst.type == gen::InstType::jumpDirect);
	entry.invalid = (inst.type == gen::InstType::invalid);
	entry.readFailure = false;

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
void gen::detail::SuperBlock::readFailure() {
	/* configure the new instruction-entry */
	Entry& entry = pList.emplace_back();
	entry.self = 0;
	entry.address = pNextAddress;
	entry.target = 0;
	entry.branches = false;
	entry.invalid = true;
	entry.readFailure = true;
	logger.debug(u8"Unable read memory for instruction [", str::As{ U"#018x", pNextAddress }, u8']');
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
	while (!raw.empty()) {
		detail::RangeIt it = raw.begin();
		size_t last = it->last;
		bool multi = false;

		/* advance the iterator to the first non-overlapping range */
		detail::RangeIt lt = it;
		for (++lt; lt != raw.end() && last >= lt->first; ++lt) {
			last = std::max(last, lt->last);
			multi = true;
		}

		/* check if multiple conflicting ranges have been found and fix them
		*	one by one and otherwise fush out the single range immediately */
		if (multi)
			fConflictCluster(raw, it, lt, it->first, last);
		else {
			pRanges.insert(detail::InstRange{ it->first, it->last, it->type });
			raw.erase(it);
		}
	}

	/* reset the iterator and counter */
	pIndex = 0;
	pIt = pRanges.begin();
}
env::guest_t gen::detail::SuperBlock::nextFetch() const {
	return pNextAddress;
}

gen::detail::InstTarget gen::detail::SuperBlock::lookup(env::guest_t target) const {
	size_t index = pTargets.size();
	size_t costs = 0;

	/* linear lookup as targets should never become really large */
	for (size_t i = 0; i < pTargets.size(); ++i) {
		if (pTargets[i].address != target || !pTargets[i].active)
			continue;

		/* check if the target can directly be used */
		size_t tCost = (pTargets[i].preNulls + pTargets[i].postNulls + (pTargets[i].conditional ? 1 : 0));
		if (tCost == 0)
			return { &pStack[pTargets[i].stack].target, 0, 0, false };

		/* check if the target is a better match than the currently found entry */
		if (tCost < costs || costs == 0) {
			costs = tCost;
			index = i;
		}
	}

	/* check if a target has been found */
	if (costs == 0)
		return { 0, 0, 0, false };
	return{ &pStack[pTargets[index].stack].target, pTargets[index].preNulls, pTargets[index].postNulls, pTargets[index].conditional };
}
bool gen::detail::SuperBlock::next() {
	pChunk.clear();

	/* check if the end has been reached (either because only one remaining invalid instruction has been encountered or because
	*	all chunks have been processed; block cannot be empty, as at least one push will occur before any chunks are fetched) */
	bool lastAndInvalid = (pIndex < pList.size() && pList[pIndex].invalid);
	if (lastAndInvalid || pIndex >= pList.size()) {
		fFinalizeBlock(lastAndInvalid);
		return false;
	}

	/* setup the new stack of targets based on the ranges */
	fPrepareStack();

	/* setup the next chunk of instructions until the next range needs to be opened or closed */
	fPrepareChunk();
	return true;
}
const std::vector<uintptr_t>& gen::detail::SuperBlock::chunk() const {
	return pChunk;
}
env::guest_t gen::detail::SuperBlock::chunkStart() const {
	return pCurrentChunk;
}
