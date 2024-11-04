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
void trans::Translator::fProcess(const detail::OpenAddress& next) {
	detail::SuperBlock block;

	/* notify the interface about the newly starting block */
	pInterface->blockStarted();

	/* construct the current super-block */
	env::guest_t address = next.address;
	do {
		/* iterate over the instruction stream and look for the end of the current strand */
		while (true) {
			trans::Instruction inst = pInterface->fetch(address);
			inst.address = address;

			/* check if the next instruction failed to be decoded */
			if (inst.type == trans::InstType::invalid)
				host::Fail(str::Format<std::u8string>(u8"Unable to decode instruction [{:#018x}]", address));
			address += inst.size;
			if (!block.push(inst))
				break;
		}
	} while (block.incomplete(address));

	/* setup the ranges of the super-block */
	block.setupRanges();

	/* setup the actual sink to the super-block and instruction-writer */
	wasm::Sink sink{ next.function };
	trans::Writer writer{ sink, block, pMemory, pContext, pMapping, pAddresses };

	/* iterate over the chunks of the super-block and produce them */
	while (true) {
		detail::InstChunk chunk = block.next(sink);
		if (chunk.count == 0)
			break;
		pInterface->produce(writer, chunk.inst, chunk.count);
	}

	/* notify the interface about the completed block */
	pInterface->blockCompleted();
}

void trans::Translator::run(env::guest_t address) {
	pAddresses.pushRoot(address);
	while (!pAddresses.empty())
		fProcess(pAddresses.start());
}
std::vector<env::BlockExport> trans::Translator::close() {
	return pAddresses.close(pMapping);
}
