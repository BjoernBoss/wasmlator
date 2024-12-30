#include "../generate.h"

static host::Logger logger{ u8"gen::block" };

gen::Block::Block(wasm::Module& mod) {
	detail::MemoryBuilder _memory;
	detail::MappingBuilder _mapping;
	detail::ProcessBuilder _process;
	detail::InteractBuilder _interact;
	detail::ContextBuilder _context;
	detail::CoreBuilder _core;

	/* bind the module to the current generator */
	gen::Instance()->setModule(&mod);

	/* initialize the block-imports */
	wasm::Prototype blockPrototype;
	wasm::Memory physical, memory;
	_core.setupBlockImports(physical, memory);
	_memory.setupBlockImports(memory, physical, pMemory);
	_mapping.setupBlockImports(blockPrototype, pMapping);
	_interact.setupBlockImports(pInteract);
	_context.setupBlockImports(memory, pContext);

	/* setup the components of the translator-members */
	pAddresses.setup(blockPrototype);
}

void gen::Block::fProcess(const detail::OpenAddress& next) {
	logger.debug(u8"Processing block at [", str::As{ U"#018x", next.address }, u8']');

	/* setup the actual sink */
	wasm::Sink sink{ next.function };
	gen::Instance()->setSink(&sink);

	/* configure the super-block and instruction-writer */
	detail::SuperBlock block{ pContext, next.address };
	gen::Writer writer{ block, pMemory, pContext, pMapping, pAddresses, pInteract };

	/* notify the interface about the newly starting block */
	gen::Instance()->translator()->started(writer, next.address);

	/* iterate over the instruction stream and look for the end of the current strand */
	gen::Instruction inst{};
	do {
		inst = gen::Instance()->translator()->fetch(block.nextFetch());
	} while (block.push(inst));

	/* setup the ranges of the super-block */
	block.setupRanges();

	/* iterate over the chunks of the super-block and produce them */
	while (block.next()) {
		env::guest_t address = block.chunkStart();
		const std::vector<uintptr_t>& chunk = block.chunk();
		gen::Instance()->translator()->produce(writer, address, chunk.data(), chunk.size());
	}

	/* notify the interface about the completed block */
	gen::Instance()->translator()->completed(writer);
	logger.debug(u8"Block at [", str::As{ U"#018x", next.address }, u8"] completed");

	/* remove the sink from the generator */
	gen::Instance()->setSink(0);
}

void gen::Block::run(env::guest_t address) {
	pAddresses.pushRoot(address);
	while (!pAddresses.empty())
		fProcess(pAddresses.start());
}
std::vector<env::BlockExport> gen::Block::close() {
	std::vector<env::BlockExport> out = pAddresses.close(pMapping);

	/* unbind the module and close it (as a precaution) */
	wasm::Module& mod = gen::Instance()->_module();
	gen::Instance()->setModule(0);
	mod.close();

	return out;
}
