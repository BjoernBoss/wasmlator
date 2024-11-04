#include "trans-translator.h"

trans::Translator::Translator(bool core, wasm::Module& mod, trans::TranslationInterface* interface, size_t maxDepth) : pAddresses{ mod, maxDepth }, pInterface{ interface } {
	if (core)
		fSetupForCore(mod);
	else
		fSetupForBlock(mod);
}

void trans::Translator::fSetupForCore(wasm::Module& mod) {
	detail::MemoryBuilder _memory;
	detail::MappingBuilder _mapping;
	detail::ContextBuilder _context;

	/* initialize the core-imports */
	_memory.setupCoreImports(mod, pMemory);
	_mapping.setupCoreImports(mod, pMapping);
	_context.setupCoreImports(mod, pContext);

	/* setup the shared components */
	env::detail::ProcessAccess _proc = env::detail::ProcessAccess{};
	wasm::Memory physical = mod.memory(u8"memory_physical", wasm::Limit{ _proc.physicalPages() }, wasm::Export{});
	wasm::Memory management = mod.memory(u8"memory_management", wasm::Limit{ _proc.managementPages(), _proc.managementPages() }, wasm::Export{});
	pMemory.physical = physical;
	pMemory.management = (pMapping.management = (pContext.management = management));

	/* setup the core-bodies */
	_memory.setupCoreBody(mod, pMemory);
	_mapping.setupCoreBody(mod, pMapping);
	_context.setupCoreBody(mod, pContext);

	/* setup the components of the translator-members */
	pAddresses.setup();
}
void trans::Translator::fSetupForBlock(wasm::Module& mod) {
	detail::MemoryBuilder _memory;
	detail::MappingBuilder _mapping;
	detail::ContextBuilder _context;

	/* setup the shared components */
	env::detail::ProcessAccess _proc = env::detail::ProcessAccess{};
	wasm::Memory physical = mod.memory(u8"memory_physical", wasm::Limit{ _proc.physicalPages() }, wasm::Import{ u8"core" });
	wasm::Memory management = mod.memory(u8"memory_management", wasm::Limit{ _proc.managementPages(), _proc.managementPages() }, wasm::Import{ u8"core" });
	pMemory.physical = physical;
	pMemory.management = (pMapping.management = (pContext.management = management));

	/* initialize the block-imports */
	_memory.setupBlockImports(mod, pMemory);
	_mapping.setupBlockImports(mod, pMapping);
	_context.setupBlockImports(mod, pContext);

	/* setup the components of the translator-members */
	pAddresses.setup();
}

size_t trans::Translator::fLookup(env::guest_t address, const std::vector<trans::Instruction>& list) const {
	size_t first = 0, last = list.size() - 1;

	while (first != last) {
		size_t index = (first + last) / 2;
		if (address > list[index].address)
			first = index + 1;
		else
			last = index;
	}

	return (list[first].address == address ? first : list.size());
}
void trans::Translator::fFetchSuperBlock(env::guest_t address, std::vector<trans::Instruction>& list) const {
	/* iterate until a true end of the super-block has been found */
	while (true) {
		/* iterate over the instruction stream and look for the end of the current strand */
		while (true) {
			trans::Instruction next = pInterface->fetch(address);
			next.address = address;

			/* check if the next instruction failed to be decoded */
			if (next.type == trans::InstType::invalid)
				host::Fail(str::Format<std::u8string>(u8"Unable to decode instruction [{:#018x}]", address));
			list.push_back(next);
			address += next.size;

			/* check if a valid end of the current strand has been found */
			if (next.type == trans::InstType::endOfBlock || next.type == trans::InstType::jumpDirect)
				break;
		}

		/* iterate over the instructions and check if any of the jumps jump to the current next
		*	address, in which case the block can be continued (iterate over all instructions again,
		*	as previous instructions might become valid after another strand has been processed)
		*
		*	Note: Ignore the case of a jump into an instruction, as it needs to be translated as a new super-block anyways */
		bool continuation = false;
		for (size_t i = 0; i < list.size(); ++i) {
			if (list[i].type != trans::InstType::jumpDirect && list[i].type != trans::InstType::conditionalDirect)
				continue;

			/* check if the instruction jumps to the current end */
			if (list[i].target != address)
				continue;
			continuation = true;
			break;
		}

		/* check if the super-block can be continued */
		if (!continuation)
			return;
	}
}
void trans::Translator::fSetupRanges(const std::vector<trans::Instruction>& list, std::set<Range>& ranges) const {
	/* collect all ranges in the instructions */
	for (size_t i = 0; i < list.size(); ++i) {
		const auto& inst = list[i];
		if (inst.type != trans::InstType::jumpDirect && inst.type != trans::InstType::conditionalDirect)
			continue;

		/* resolve the jump-target and check if it lies in the current super-block */
		size_t target = fLookup(inst.target, list);
		if (target == list.size())
			continue;

		/* check if the direction is backwards (also the case for equal, as a jump to itself is considered a backwards jump) */
		bool backwards = (target <= i);

		/* construct the range as minimally necessary (i.e. must at least wrap both the instruction at index [first] and at index [last]) */
		size_t first = std::min<size_t>(i, target);
		size_t last = std::max<size_t>(i, target);
		if (!backwards)
			--last;
		ranges.insert({ first, last, i, backwards });
	}
}
bool trans::Translator::fExpandRanges(std::set<Range>& ranges) const {
	/* expand all ranges, such that the start is moved as far out as possible
	*	in order to ensure all ranges are considered nested in some form
	*
	*	Note: This will not fix irreducible control-flows, they will be ignored for now and handled later */
	bool conflicts = false;

	/* iterate over the ranges, and check all of the overlapping ranges for conflicts, and try to expand them */
	std::set<Range>::iterator it = ranges.begin();
	while (it != ranges.end()) {
		bool advance = true;

		/* iterate over all upcoming overlapping ranges */
		for (std::set<Range>::iterator nt = std::next(it); nt != ranges.end() && it->last >= nt->first; ++nt) {
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
			Range next = { it->first, nt->last, (iremove ? it->origin : nt->origin), iremove };

			/* swap the ranges out and restart the iteration, as some changes might now collide with previously resolved ranges */
			ranges.erase(iremove ? it : nt);
			ranges.insert(next);
			advance = false;
			break;
		}

		/* check if the iterator can be advanced or needs to be reset */
		it = (advance ? std::next(it) : ranges.begin());
	}

	/* return whether or not irreducible control-flows were encountered */
	return conflicts;
}
void trans::Translator::fFixRanges(std::set<Range>& ranges) const {
	host::Fail(u8"Irreducible control-flow within a super-block cannot yet be translated!");
}

void trans::Translator::fProcess(const detail::OpenAddress& next) {
	/* construct the current super-block */
	std::vector<trans::Instruction> list;
	fFetchSuperBlock(next.address, list);

	/* create the set of jump-ranges */
	std::set<Range> ranges;
	fSetupRanges(list, ranges);

	/* expand all ranges to ensure as many as possible are nesting and afterwards fix irreducible control-flow issues within the ranges */
	if (fExpandRanges(ranges))
		fFixRanges(ranges);

	/* setup the actual sink to the super-block and instruction-writer */
	wasm::Sink sink{ next.function };
	trans::Writer writer{ sink, pMemory, pContext, pMapping, pAddresses };

	/* iterate over the instruction-list and push it out */
	size_t index = 0;
	std::set<Range>::const_iterator it = ranges.begin();
	std::list<std::variant<wasm::Loop, wasm::Block>> stack;
	while (index < list.size()) {
		/* check if the remaining instructions can just be written out */
		if (it == ranges.end()) {
			pInterface->produce(writer, list.data() + index, list.size() - index);
			break;
		}

		stack.emplace_back(wasm::Loop{ sink });
		stack.emplace_back(wasm::Block{ sink });

		wasm::Target _target;




		/* compute the number of instructions in this basic-block */
		//size_t count = (it == ranges.end() ? list.size() - index : )


	}

}

void trans::Translator::run(env::guest_t address) {
	pAddresses.pushRoot(address);
	while (!pAddresses.empty())
		fProcess(pAddresses.start());
}
std::vector<env::BlockExport> trans::Translator::close() {
	return pAddresses.close(pMapping);
}
