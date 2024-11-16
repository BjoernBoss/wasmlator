#include "gen-translator.h"
#include "../environment/env-process.h"

gen::Translator::Translator(wasm::Module& mod) : pAddresses{ mod } {
	detail::MemoryBuilder _memory;
	detail::MappingBuilder _mapping;
	detail::ContextBuilder _context;
	detail::InteractBuilder _interact;
	detail::CoreBuilder _core;

	/* initialize the block-imports */
	wasm::Prototype blockPrototype;
	wasm::Memory physical, management;
	_core.setupBlockImports(mod, physical, management);
	_memory.setupBlockImports(mod, management, physical, pMemory);
	_mapping.setupBlockImports(mod, blockPrototype, pMapping);
	_context.setupBlockImports(mod, management, pContext);
	_interact.setupBlockImports(mod, pInteract);

	/* setup the components of the translator-members */
	pAddresses.setup(blockPrototype);
}

void gen::Translator::fProcess(const detail::OpenAddress& next) {
	detail::SuperBlock block;

	/* notify the interface about the newly starting block */
	env::Instance()->specification().translationStarted();

	/* construct the current super-block */
	env::guest_t address = next.address;
	do {
		/* iterate over the instruction stream and look for the end of the current strand */
		while (true) {
			gen::Instruction inst = env::Instance()->specification().fetchInstruction(address);
			inst.address = address;

			/* check if the next instruction failed to be decoded */
			if (inst.type == gen::InstType::invalid)
				host::Fatal(str::Format<std::u8string>(u8"Unable to decode instruction [{:#018x}]", address));
			address += inst.size;
			if (!block.push(inst))
				break;
		}
	} while (block.incomplete(address));

	/* setup the ranges of the super-block */
	block.setupRanges();

	/* setup the actual sink to the super-block and instruction-writer */
	wasm::Sink sink{ next.function };
	gen::Writer writer{ sink, block, pMemory, pContext, pMapping, pAddresses, pInteract };

	/* iterate over the chunks of the super-block and produce them */
	while (true) {
		detail::InstChunk chunk = block.next(sink);
		if (chunk.count == 0)
			break;
		env::Instance()->specification().produceInstructions(writer, chunk.inst, chunk.count);
	}

	/* notify the interface about the completed block */
	env::Instance()->specification().translationCompleted();
}

void gen::Translator::run(env::guest_t address) {
	pAddresses.pushRoot(address);
	while (!pAddresses.empty())
		fProcess(pAddresses.start());
}
std::vector<env::BlockExport> gen::Translator::close() {
	return pAddresses.close(pMapping);
}
