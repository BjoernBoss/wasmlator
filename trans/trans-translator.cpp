#include "trans-translator.h"

trans::Translator::Translator(wasm::Module& mod, trans::TranslationInterface* interface, size_t maxDepth) : pAddresses{ mod, maxDepth }, pInterface{ interface } {}

trans::Translator trans::Translator::CoreModule(wasm::Module& mod, trans::TranslationInterface* interface, size_t maxDepth) {
	trans::Translator out{ mod, interface, maxDepth };
	detail::MemoryBuilder _memory;
	detail::MappingBuilder _mapping;
	detail::ContextBuilder _context;

	/* initialize the core-imports */
	_memory.setupCoreImports(mod, out.pMemory);
	_mapping.setupCoreImports(mod, out.pMapping);
	_context.setupCoreImports(mod, out.pContext);

	/* setup the shared components */
	env::detail::ProcessAccess _proc = env::detail::ProcessAccess{};
	wasm::Memory physical = mod.memory(u8"memory_physical", wasm::Limit{ _proc.physicalPages() }, wasm::Export{});
	wasm::Memory management = mod.memory(u8"memory_management", wasm::Limit{ _proc.managementPages(), _proc.managementPages() }, wasm::Export{});
	out.pMemory.physical = physical;
	out.pMemory.management = (out.pMapping.management = (out.pContext.management = management));

	/* setup the core-bodies */
	_memory.setupCoreBody(mod, out.pMemory);
	_mapping.setupCoreBody(mod, out.pMapping);
	_context.setupCoreBody(mod, out.pContext);

	/* setup the components of the translator-members */
	out.pAddresses.setup();
	return out;
}
trans::Translator trans::Translator::BlockModule(wasm::Module& mod, trans::TranslationInterface* interface, size_t maxDepth) {
	trans::Translator out{ mod, interface, maxDepth };
	detail::MemoryBuilder _memory;
	detail::MappingBuilder _mapping;
	detail::ContextBuilder _context;

	/* setup the shared components */
	env::detail::ProcessAccess _proc = env::detail::ProcessAccess{};
	wasm::Memory physical = mod.memory(u8"memory_physical", wasm::Limit{ _proc.physicalPages() }, wasm::Import{ u8"core" });
	wasm::Memory management = mod.memory(u8"memory_management", wasm::Limit{ _proc.managementPages(), _proc.managementPages() }, wasm::Import{ u8"core" });
	out.pMemory.physical = physical;
	out.pMemory.management = (out.pMapping.management = (out.pContext.management = management));

	/* initialize the block-imports */
	_memory.setupBlockImports(mod, out.pMemory);
	_mapping.setupBlockImports(mod, out.pMapping);
	_context.setupBlockImports(mod, out.pContext);

	/* setup the components of the translator-members */
	out.pAddresses.setup();
	return out;
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

		/* add the range to the queue (ensure backwards is less-equal, as jump to itself is considered a backwards-jump) */
		size_t first = std::min<size_t>(i, target);
		size_t last = std::max<size_t>(i, target);
		bool backwards = (target <= i);
		ranges.insert({ first, last, backwards });
	}

	/* ensure all boundary-overlapping ranges of the same direction have a common start */
	std::set<Range>::iterator it = ranges.begin();
	while (it != ranges.end()) {
		/* check if any upcoming range has its start within the current range, but ends outside of it, while sharing a direction */
		for (std::set<Range>::iterator ot = std::next(it); ot != ranges.end(); ++ot) {
			if (ot->last >= it->last)
				break;
			if (ot->backwards != it->backwards)
				continue;


		}
	}
}

void trans::Translator::fProcess(const detail::OpenAddress& next) {
	/* construct the current super-block */
	std::vector<trans::Instruction> list;
	fFetchSuperBlock(next.address, list);

	/* create the set of jump-ranges */
	std::set<Range> ranges;
	fSetupRanges(list, ranges);

	/* setup the actual sink to the super-block and instruction-writer */
	wasm::Sink sink{ next.function };
	trans::Writer writer{ sink, pMemory, pContext, pMapping, pAddresses };

	/* iterate over the instruction-list and push it out */
	size_t index = 0;
	std::set<Range>::const_iterator it = ranges.begin();
	while (index < list.size()) {
		/* check if the remaining instructions can just be written out */
		if (it == ranges.end()) {
			pInterface->produce(writer, list.data() + index, list.size() - index);
			break;
		}



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
