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

	/* find the next irreducible jump to be resolved (just pick the first one, as it does not make a difference as there
	*	will typically only be one - there must be one, as the other patterns would otherwise have resolved the issue)
	*
	*	Note: due to the nature of web-assembly loop constriants, irreducible control-flows can only be resolved
	*	by expanding a backward jump all the way to the top and adding an additional forward jump, because the other
	*	solution would require a block, which afterwards jumps back up to the loop-start of a nested loop... */
	detail::RangeIt irreducibleIt = end;
	for (detail::RangeIt it = begin; it != end; ++it) {
		/* check if this is a candidate for a conflict */
		if (it->type != detail::RangeType::backwards || it->last != last)
			continue;
		irreducibleIt = it;
		break;
	}

	/* remove the resolved range and insert the long range to the output (it spans over the entire
	*	conflict, and can therefore not induce another conflict) and the short range back to the
	*	input (as it might still be part of conflicts) - output will be cleaned up afterwards */
	size_t target = irreducibleIt->first - 1;
	set.erase(irreducibleIt);
	set.insert(detail::InstRange{ first, target, detail::RangeType::conditional });
	pRanges.insert(detail::InstRange{ first, last, detail::RangeType::backwards });
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
			if (pStack[j - 1].type == detail::RangeType::backwards && pStack[j - 1].first == pTargets[i - 1].first) {
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

	/* check if at least one conditional starts at this index, in which case a conditional branch needs to be inserted */
	size_t totalConditionals = 0, seenConditionals = 0;
	for (auto it = pIt; it != pRanges.end() && pIndex == it->first; ++it) {
		if (it->type == detail::RangeType::conditional)
			++totalConditionals;
	}

	/* setup the default-exit index and initial branch to the default-exit */
	size_t defaultExit = (totalConditionals <= 1 ? 0 : totalConditionals);
	if (totalConditionals > 0)
		gen::Add[I::U32::Const(defaultExit)];

	/* check if the next ranges need to be opened (there must always be a backward-jump before a conditional) */
	size_t lastBackward = pStack.size(), startOfstack = pStack.size();
	while (pIt != pRanges.end() && pIndex == pIt->first) {
		std::initializer_list<wasm::Type> params = (seenConditionals < totalConditionals ?
			std::initializer_list<wasm::Type>{ wasm::Type::i32 } :
			std::initializer_list<wasm::Type>{});

		/* create the next wasm-target object and setup the last-backward jump, which qualifies for
		*	jump-target for all upcoming conditionals, or patch the number of seen backward conditionals */
		if (pIt->type == detail::RangeType::backwards) {
			lastBackward = pStack.size();
			pStack.push_back({ wasm::Loop{ gen::Sink, u8"", params, {} }, pIt->first, pIt->last, pIt->type });
		}
		else
			pStack.push_back({ wasm::Block{ gen::Sink, u8"", params, {} }, pIt->first, pIt->last, pIt->type });

		/* populate the target attributes with the first unconditional primitive entry */
		Target& target = allocTarget();
		target.address = pList[pIt->type == detail::RangeType::backwards ? pIt->first : pIt->last + 1].address;
		target.stack = pStack.size() - 1;
		target.first = pIt->first;
		target.index = defaultExit;
		target.conditional = (pIt->type == detail::RangeType::backwards && seenConditionals < totalConditionals);
		target.active = true;

		/* check if the conditional entry needs to be added as well */
		if (pIt->type != detail::RangeType::conditional) {
			++pIt;
			continue;
		}

		/* setup the conditional entry */
		Target& cond = allocTarget();
		cond.address = target.address;
		cond.stack = lastBackward;
		cond.first = target.first;
		cond.index = (totalConditionals == 1 ? 1 : seenConditionals);
		cond.conditional = true;
		cond.active = true;
		++pIt;

		/* check if this was the last conditional, in which case the jump can now be added */
		if (++seenConditionals < totalConditionals)
			continue;

		/* check if a single branch can be added */
		if (totalConditionals == 1) {
			gen::Add[I::Branch::If(pStack.back().target)];
			continue;
		}

		/* collect the list of conditionals */
		std::vector<wasm::WTarget> targets;
		for (size_t i = startOfstack; i < pStack.size(); ++i) {
			if (pStack[i].type == detail::RangeType::conditional)
				targets.push_back(pStack[i].target);
		}

		/* add the wrapping default-exit and the conditional list of all targets */
		wasm::Block _default{ gen::Sink, u8"", { wasm::Type::i32 }, {} };
		gen::Add[I::Branch::Table(targets, _default)];
	}
}
void gen::detail::SuperBlock::fPrepareChunk(bool single) {
	/* check if only a single instruction is to be produced */
	if (single) {
		pCurrentChunk = pList[pIndex].address;
		pChunk.push_back(pList[pIndex++].self);
		return;
	}

	/* extract the number of instructions until the next range either needs to start or has to be closed */
	size_t last = pList.size() - 1;
	if (!pStack.empty() && pStack.back().last < last)
		last = pStack.back().last;
	if (pIt != pRanges.end() && pIt->first <= last)
		last = pIt->first - 1;

	/* check if the chunk contains a potential last un-decodable instruction, and remove it from this chunk
	*	(cannot be the only instruction, as the chunk would otherwise not have been produced at all) */
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
	/* create the set of jump-ranges */
	std::set<detail::InstRange> raw = fSetupRanges();

	/* iterate over the ranges and find all conflicting clusters, fix them, and write them out to the final ranges */
	while (!raw.empty()) {
		detail::RangeIt it = raw.begin();
		size_t last = it->last;

		/* advance the iterator to the first non-overlapping range (i.e. find the end of the current overlapping cluster) */
		detail::RangeIt lt = it;
		for (++lt; lt != raw.end() && last >= lt->first; ++lt)
			last = std::max(last, lt->last);

		/* resolve the overlapping conflict */
		fConflictCluster(raw, it, lt, it->first, last);
	}

	/* iterate over the ranges and remove all backward jumps, which jump to the same
	*	target and collect all forward targets (necessary to cleanup forward jumps) */
	std::unordered_set<size_t> forward, conditional;
	size_t count = pRanges.size();
	for (detail::RangeIt it = pRanges.begin(); it != pRanges.end(); ++it) {
		/* check if this is a backward-jump to be cleaned up */
		if (it->type != detail::RangeType::backwards)
			continue;

		/* find all useless jumps to the same target (all later ranges will automatically have a shorter reach) */
		detail::RangeIt ot = std::next(it);
		while (ot != pRanges.end()) {
			if (ot->type == it->type && ot->first == it->first)
				ot = pRanges.erase(ot);
			else
				++ot;
		}
	}
	logger.trace(u8"Ranges: ", pRanges.size(), u8" (Reduction: ", count - pRanges.size(), u8')');

	/* reset the iterator and counter */
	pIndex = 0;
	pIt = pRanges.begin();
}
env::guest_t gen::detail::SuperBlock::nextFetch() const {
	return pNextAddress;
}

gen::detail::InstTarget gen::detail::SuperBlock::lookup(env::guest_t target) const {
	size_t index = pTargets.size();

	/* linear lookup as targets should never become really large */
	for (size_t i = 0; i < pTargets.size(); ++i) {
		if (pTargets[i].address != target || !pTargets[i].active)
			continue;

		/* check if the target can directly be used */
		if (!pTargets[i].conditional)
			return { &pStack[pTargets[i].stack].target, 0, false };
		index = i;
	}

	/* check if a target has been found */
	if (index == pTargets.size())
		return { 0, 0, false };
	return{ &pStack[pTargets[index].stack].target, pTargets[index].index, pTargets[index].conditional };
}
bool gen::detail::SuperBlock::next(bool single) {
	pChunk.clear();

	/* check if the end has been reached (either because only one remaining invalid instruction has been encountered or because
	*	all chunks have been processed; block cannot be empty, as at least one push will occur before any chunks are fetched) */
	if ((pIndex < pList.size() && pList[pIndex].invalid) || pIndex >= pList.size()) {
		pCurrentChunk = pNextAddress;
		return false;
	}

	/* setup the new stack of targets based on the ranges */
	fPrepareStack();

	/* setup the next chunk of instructions until the next range needs to be opened or closed */
	fPrepareChunk(single);
	return true;
}
void gen::detail::SuperBlock::finalize() {
	pStack.clear();

	/* check if the not-decodable/not-readable stub needs to be added */
	if (pIndex < pList.size() && pList[pIndex].invalid) {
		if (pList[pIndex].readFailure)
			pContext.makeNotReadable(pNextAddress);
		else
			pContext.makeNotDecodable(pNextAddress);
		gen::Add[I::Unreachable()];
	}

	/* add the not-reachable stub, as this address should never be reached as the super-block would otherwise have been continued */
	else {
		pContext.makeNotReachable(pNextAddress);
		gen::Add[I::Unreachable()];
	}
}
const std::vector<uintptr_t>& gen::detail::SuperBlock::chunk() const {
	return pChunk;
}
env::guest_t gen::detail::SuperBlock::chunkStart() const {
	return pCurrentChunk;
}
