/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025 Bjoern Boss Henrichsen */
#include "../generate.h"
#include "../../environment/environment.h"

static util::Logger logger{ u8"gen::block" };

bool gen::detail::BlockAccess::Setup(detail::BlockState& state) {
	if (gen::Instance()->trace() != gen::TraceType::none) {
		state.blockCallbackId = env::Instance()->interact().defineCallback([](uint64_t addr) -> uint64_t {
			logger.debug(u8"Entering Block [", str::As{ U"#018x", addr }, u8']');
			return 0;
			});
		state.chunkCallbackId = env::Instance()->interact().defineCallback([](uint64_t addr) -> uint64_t {
			logger.debug(u8"Chunk Trace [", str::As{ U"#018x", addr }, u8']');
			return 0;
			});
		state.instCallbackId = env::Instance()->interact().defineCallback([](uint64_t addr) -> uint64_t {
			logger.debug(u8"Trace [", str::As{ U"#018x", addr }, u8']');
			return 0;
			});
	}

	if (gen::Instance()->debugCheck()) {
		state.debugCheckCallbackId = env::Instance()->interact().defineCallback([](uint64_t addr) -> uint64_t {
			detail::GeneratorAccess::DebugCheck(addr);
			return 0;
			});
	}
	return true;
}


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
	logger.trace(u8"Processing block at [", str::As{ U"#018x", next.address }, u8']');

	/* setup the actual sink */
	wasm::Sink sink{ next.function };
	gen::Instance()->setSink(&sink);

	/* configure the super-block and instruction-writer */
	detail::SuperBlock block{ pContext, next.address };
	gen::Writer writer{ block, pMemory, pContext, pMapping, pAddresses, pInteract };
	detail::GeneratorAccess::SetWriter(&writer);

	/* check if block-tracing is enabled */
	if (gen::Instance()->trace() != gen::TraceType::none) {
		gen::Add[I::U64::Const(next.address)];
		gen::Make->invokeParam(detail::GeneratorAccess::GetBlock()->blockCallbackId);
		gen::Add[I::Drop()];
	}

	/* notify the interface about the newly starting block */
	detail::GeneratorAccess::Get()->started(next.address);

	/* iterate over the instruction stream and look for the end of the current strand */
	gen::Instruction inst{};
	try {
		do {
			inst = detail::GeneratorAccess::Get()->fetch(block.nextFetch());
		} while (block.push(inst));
	}

	/* check if a memory-fault occurred (i.e. trying to execute non-executable memory - no
	*	need to add separate handling if the memory changes afterwards, as this would require
	*	flushing of the instruction-cache, thereby removing this stub as well - invalid
	*	instructions will not be passed to the translator, therefore self can just be null) */
	catch (const env::MemoryFault&) {
		block.readFailure();
	}

	/* setup the ranges of the super-block */
	block.setupRanges();

	/* iterate over the chunks of the super-block and produce them */
	bool singleInstructions = (gen::Instance()->debugCheck() || gen::Instance()->trace() == gen::TraceType::instruction);
	while (block.next(singleInstructions)) {
		env::guest_t address = block.chunkStart();

		/* add the debug-check stub */
		if (gen::Instance()->debugCheck()) {
			gen::Add[I::U64::Const(address)];
			gen::Make->invokeParam(detail::GeneratorAccess::GetBlock()->debugCheckCallbackId);
			gen::Add[I::Drop()];
		}

		/* check if chunk-tracing or instruction tracing is enabled */
		if (gen::Instance()->trace() == gen::TraceType::chunk || gen::Instance()->trace() == gen::TraceType::instruction) {
			gen::Add[I::U64::Const(address)];
			gen::Make->invokeParam(detail::GeneratorAccess::GetBlock()->chunkCallbackId);
			gen::Add[I::Drop()];
		}
		if (gen::Instance()->trace() == gen::TraceType::instruction) {
			gen::Add[I::U64::Const(address)];
			gen::Make->invokeParam(detail::GeneratorAccess::GetBlock()->instCallbackId);
			gen::Add[I::Drop()];
		}

		/* produce the actual instructions of the chunk */
		const std::vector<uintptr_t>& chunk = block.chunk();
		detail::GeneratorAccess::Get()->produce(address, chunk.data(), chunk.size());
	}

	/* add final debug-check stub */
	if (gen::Instance()->debugCheck()) {
		gen::Add[I::U64::Const(block.chunkStart())];
		gen::Make->invokeParam(detail::GeneratorAccess::GetBlock()->debugCheckCallbackId);
		gen::Add[I::Drop()];
	}

	/* finalize the generated block */
	block.finalize();

	/* notify the interface about the completed block */
	detail::GeneratorAccess::Get()->completed();

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
