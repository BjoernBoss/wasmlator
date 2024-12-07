#include "gen-block.h"

gen::Block::Block(wasm::Module& mod, gen::Translator* translator, size_t maxDepth) : pAddresses{ mod, maxDepth }, pTranslator{ translator } {
	detail::MemoryBuilder _memory;
	detail::MappingBuilder _mapping;
	detail::ContextBuilder _context;
	detail::InteractBuilder _interact;
	detail::CoreBuilder _core;

	/* initialize the block-imports */
	wasm::Prototype blockPrototype;
	wasm::Memory physical, memory;
	_core.setupBlockImports(mod, physical, memory);
	_memory.setupBlockImports(mod, memory, physical, pMemory);
	_mapping.setupBlockImports(mod, blockPrototype, pMapping);
	_context.setupBlockImports(mod, memory, pContextShared, pContext);
	_interact.setupBlockImports(mod, pInteract);

	/* setup the components of the translator-members */
	pAddresses.setup(blockPrototype);
}

void gen::Block::fProcess(const detail::OpenAddress& next) {
	host::Debug(str::u8::Format(u8"Processing block at [{:#018x}]", next.address));
	detail::SuperBlock block{ pContextShared, next.address };

	/* setup the actual sink to the super-block and instruction-writer */
	wasm::Sink sink{ next.function };
	gen::Writer writer{ sink, block, pMemory, pContext, pMapping, pAddresses, pInteract };

	/* notify the interface about the newly starting block */
	pTranslator->started(writer);

	/* iterate over the instruction stream and look for the end of the current strand */
	gen::Instruction inst{};
	do {
		inst = pTranslator->fetch(block.nextFetch());
	} while (block.push(inst));

	/* setup the ranges of the super-block */
	block.setupRanges();

	/* iterate over the chunks of the super-block and produce them */
	while (block.next(sink)) {
		env::guest_t address = block.chunkStart();
		const std::vector<uintptr_t>& chunk = block.chunk();
		pTranslator->produce(writer, address, chunk.data(), chunk.size());
	}

	/* notify the interface about the completed block */
	pTranslator->completed(writer);
	host::Debug(str::u8::Format(u8"Block at [{:#018x}] completed", next.address));
}

void gen::Block::run(env::guest_t address) {
	pAddresses.pushRoot(address);
	while (!pAddresses.empty())
		fProcess(pAddresses.start());
}
std::vector<env::BlockExport> gen::Block::close() {
	return pAddresses.close(pMapping);
}
