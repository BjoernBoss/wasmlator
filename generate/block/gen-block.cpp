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
	detail::GeneratorAccess::SetWriter(&writer);

	/* notify the interface about the newly starting block */
	detail::GeneratorAccess::Get()->started(next.address);

	/* iterate over the instruction stream and look for the end of the current strand */
	gen::Instruction inst{};
	do {
		inst = detail::GeneratorAccess::Get()->fetch(block.nextFetch());
	} while (block.push(inst));

	/* setup the ranges of the super-block */
	block.setupRanges();

	/* iterate over the chunks of the super-block and produce them */
	while (block.next()) {
		env::guest_t address = block.chunkStart();
		const std::vector<uintptr_t>& chunk = block.chunk();
		detail::GeneratorAccess::Get()->produce(address, chunk.data(), chunk.size());
	}

	/* notify the interface about the completed block */
	detail::GeneratorAccess::Get()->completed();
	logger.debug(u8"Block at [", str::As{ U"#018x", next.address }, u8"] completed");

	/* remove the writer and sink from the generator */
	detail::GeneratorAccess::SetWriter(0);
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
	gen::Instance()->setModule(0)->close();
	return out;
}
