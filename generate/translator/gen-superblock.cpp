#include "gen-superblock.h"

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
void gen::detail::SuperBlock::fSetupRanges() {
	/* collect all ranges in the instructions */
	for (size_t i = 0; i < pList.size(); ++i) {
		const auto& inst = pList[i];
		if (inst.type != gen::InstType::jumpDirect && inst.type != gen::InstType::conditionalDirect)
			continue;

		/* resolve the jump-target and check if it lies in the current super-block */
		size_t target = fLookup(inst.target);
		if (target == pList.size())
			continue;

		/* check if the direction is backwards (also the case for equal, as a jump to itself is considered a backwards jump) */
		bool backwards = (target <= i);

		/* construct the range as minimally necessary (i.e. must at least wrap both the instruction at index [first] and at index [last]) */
		size_t first = std::min<size_t>(i, target);
		size_t last = std::max<size_t>(i, target);
		if (!backwards)
			--last;
		pRanges.insert({ first, last, i, backwards });
	}
}
bool gen::detail::SuperBlock::fExpandRanges() {
	/* expand all ranges, such that the start is moved as far out as possible
	*	in order to ensure all ranges are considered nested in some form
	*
	*	Note: This will not fix irreducible control-flows, they will be ignored for now and handled later */
	bool conflicts = false;

	/* iterate over the ranges, and check all of the overlapping ranges for conflicts, and try to expand them */
	std::set<detail::InstRange>::iterator it = pRanges.begin();
	while (it != pRanges.end()) {
		bool advance = true;

		/* iterate over all upcoming overlapping ranges */
		for (std::set<detail::InstRange>::iterator nt = std::next(it); nt != pRanges.end() && it->last >= nt->first; ++nt) {
			/* check if the upcoming range is already fully nested within the current range */
			if (it->last >= nt->last)
				continue;

			/* compute the possible move-distance of the current range's end and the upcoming range's start (if none
			*	is possible, this is an irreducible control-flow, which needs to be handled separately later) */
			size_t imove = (it->backwards ? nt->last - it->last : 0);
			size_t nmove = (nt->backwards ? 0 : nt->first - it->first);
			if (imove == 0 && nmove == 0) {
				conflicts = true;
				continue;
			}

			/* select the option with the lower distance changed to be moved */
			bool iremove = (imove > 0 && (nmove == 0 || imove < nmove));
			detail::InstRange next = { it->first, nt->last, (iremove ? it->origin : nt->origin), iremove };

			/* swap the ranges out and restart the iteration, as some changes might now collide with previously resolved ranges */
			pRanges.erase(iremove ? it : nt);
			pRanges.insert(next);
			advance = false;
			break;
		}

		/* check if the iterator can be advanced or needs to be reset */
		it = (advance ? std::next(it) : pRanges.begin());
	}

	/* return whether or not irreducible control-flows were encountered */
	return conflicts;
}
void gen::detail::SuperBlock::fFixRanges() {
	host::Fatal(u8"Irreducible control-flow within a super-block cannot yet be translated!");
}

bool gen::detail::SuperBlock::push(const gen::Instruction& inst) {
	pList.push_back(inst);

	/* check if the current strand can be continued */
	return (inst.type != gen::InstType::endOfBlock && inst.type != gen::InstType::jumpDirect);
}
bool gen::detail::SuperBlock::incomplete(env::guest_t address) const {
	/* iterate over the instructions and check if any of the jumps jump to the current next
	*	address, in which case the block can be continued (iterate over all instructions again,
	*	as previous instructions might become valid after another strand has been processed)
	*
	*	Note: Ignore the case of a jump into an instruction, as it needs to be translated as a new super-block anyways */
	for (size_t i = 0; i < pList.size(); ++i) {
		if (pList[i].type != gen::InstType::jumpDirect && pList[i].type != gen::InstType::conditionalDirect)
			continue;

		/* check if the instruction jumps to the current end */
		if (pList[i].target == address)
			return true;
	}
	return false;
}
void gen::detail::SuperBlock::setupRanges() {
	/* create the set of jump-ranges */
	fSetupRanges();

	/* expand all ranges to ensure as many as possible are nesting and
	*	afterwards fix irreducible control-flow issues within the ranges */
	if (fExpandRanges())
		fFixRanges();

	/* reset the iterator and counter */
	pIndex = 0;
	pIt = pRanges.begin();
}

const wasm::Target* gen::detail::SuperBlock::lookup(env::guest_t address) const {
	/* linear lookup as stack should never become really large */
	for (size_t i = 0; i < pStack.size(); ++i) {
		if (pStack[i].address == address)
			return &pStack[i].target;
	}
	return 0;
}
gen::detail::InstChunk gen::detail::SuperBlock::next(wasm::Sink& sink) {
	/* check if the end has been reached */
	if (pIndex >= pList.size()) {
		pStack.clear();
		return { 0, 0 };
	}

	/* pop all closed ranges */
	while (!pStack.empty() && pIndex > pStack.back().last)
		pStack.pop_back();

	/* check if the next ranges need to be opened */
	while (pIt != pRanges.end() && pIndex == pIt->first) {
		if (pIt->backwards)
			pStack.push_back({ wasm::Loop{ sink }, pList[pIt->origin].address, pIt->last });
		else
			pStack.push_back({ wasm::Block{ sink }, pList[pIt->origin].address, pIt->last });
		++pIt;
	}

	/* extract the number of instructions until the next range either needs to start or has to be closed */
	size_t last = pList.size() - 1;
	if (!pStack.empty() && pStack.back().last < last)
		last = pStack.back().last;
	if (pIt != pRanges.end() && pIt->first <= last)
		last = pIt->first - 1;

	/* setup the chunk and advance the index */
	detail::InstChunk chunk{ pList.data() + pIndex, (last - pIndex) + 1 };
	pIndex = last + 1;
	return chunk;
}
