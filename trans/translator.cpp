#include "translator.h"

trans::Translator::Translator(wasm::Module& mod, trans::TranslationInterface* interface, size_t maxDepth) : pModule{ mod }, pInterface{ interface }, pMaxDepth{ maxDepth } {}

trans::Translator trans::Translator::CoreModule(wasm::Module& mod, trans::TranslationInterface* interface, size_t maxDepth) {
	trans::Translator out{ mod, interface, maxDepth };

	detail::MemoryBuilder{}.setupCoreImports(mod, out.pMemory);
	detail::MappingBuilder{}.setupCoreImports(mod, out.pMapping);
	detail::ContextBuilder{}.setupCoreImports(mod, out.pContext);

	/* setup the shared components */
	env::detail::ProcessAccess _proc = env::detail::ProcessAccess{};
	wasm::Memory physical = mod.memory(u8"memory_physical", wasm::Limit{ _proc.physicalPages() }, wasm::Export{});
	wasm::Memory management = mod.memory(u8"memory_management", wasm::Limit{ _proc.managementPages(), _proc.managementPages() }, wasm::Export{});

	out.pMemory.physical = physical;
	out.pMemory.management = (out.pMapping.management = (out.pContext.management = management));

	detail::MemoryBuilder{}.setupCoreBody(mod, out.pMemory);
	detail::MappingBuilder{}.setupCoreBody(mod, out.pMapping);
	detail::ContextBuilder{}.setupCoreBody(mod, out.pContext);

	return out;
}
trans::Translator trans::Translator::BlockModule(wasm::Module& mod, trans::TranslationInterface* interface, size_t maxDepth) {
	trans::Translator out{ mod, interface, maxDepth };

	/* setup the shared components */
	env::detail::ProcessAccess _proc = env::detail::ProcessAccess{};
	wasm::Memory physical = mod.memory(u8"memory_physical", wasm::Limit{ _proc.physicalPages() }, wasm::Import{ u8"core" });
	wasm::Memory management = mod.memory(u8"memory_management", wasm::Limit{ _proc.managementPages(), _proc.managementPages() }, wasm::Import{ u8"core" });

	out.pMemory.physical = physical;
	out.pMemory.management = (out.pMapping.management = (out.pContext.management = management));

	detail::MemoryBuilder{}.setupBlockImports(mod, out.pMemory);
	detail::MappingBuilder{}.setupBlockImports(mod, out.pMapping);
	detail::ContextBuilder{}.setupBlockImports(mod, out.pContext);

	return out;
}


size_t trans::Translator::fLookup(env::guest_t address, const std::vector<Translated>& list) const {
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
void trans::Translator::fFetchSuperBlock(env::guest_t address, std::vector<Translated>& list) {
	/* iterate until a true end of the super-block has been found */
	while (true) {
		bool startOfBlock = (list.size() > 0);

		/* iterate over the instruction stream and look for the end of the current strand */
		while (true) {
			trans::Instruction next = pInterface->fetch(address);

			/* check if the next instruction failed to be decoded */
			if (next.type == trans::InstType::invalid)
				host::Fail(str::Format<std::u8string>(u8"Unable to decode instruction [{:#018x}]", address));
			list.push_back({ next, address, 0, false, startOfBlock });
			address += next.size;
			startOfBlock = false;

			/* check if a valid end of the current strand has been found */
			if (next.type == trans::InstType::endOfBlock || next.type == trans::InstType::jumpDirect || next.type == trans::InstType::jumpIndirect)
				break;
		}

		/* iterate over the instructions and check if any of the jumps jump into the instructions themselves,
		*	or even to the current next address, in which case the block can be continued (iterate over all instructions
		*	again, as previous instructions might become valid after another strand has been processed) */
		bool continuation = false;
		for (size_t i = 0; i < list.size(); ++i) {
			/* check if the instruction can be considered and has not already been resolved as jumping locally */
			if (list[i].inst.type != trans::InstType::jumpDirect && list[i].inst.type != trans::InstType::conditionalDirect)
				continue;
			if (list[i].local)
				continue;

			/* check if the instruction jumps to the current end */
			if (list[i].inst.target == address) {
				continuation = true;
				list[i].index = list.size();
				list[i].local = true;
				continue;
			}

			/* check if the instruction jumps into the current list of instructions */
			list[i].index = fLookup(list[i].inst.target, list);
			if (list[i].index < list.size()) {
				list[i].local = true;
				list[list[i].index].startOfBlock = true;
			}
		}

		/* check if the super-block can be continued */
		if (!continuation)
			return;
	}
}


// ignore the case of a jump into an instruction, as it needs to be translated as a new super-block

void trans::Translator::fPush(env::guest_t address, size_t depth) {
	/* check if the depth-limit has been reached */

	/* check if the address has already been */
}
void trans::Translator::fProcess(env::guest_t address, size_t depth) {
	std::vector<Translated> list;

	fFetchSuperBlock(address, list);

}

void trans::Translator::run(env::guest_t address) {
	fPush(address, 0);

	/* process the queue until all open addresses have been processed */
	while (!pQueue.empty()) {
		Entry entry = pQueue.front();
		pQueue.pop();
		fProcess(entry.address, entry.depth + 1);
	}
}
void trans::Translator::close() {

}
