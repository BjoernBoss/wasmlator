#include "gen-core.h"
#include "../memory/memory-builder.h"
#include "../context/context-builder.h"
#include "../mapping/mapping-builder.h"
#include "../interact/interact-builder.h"
#include "../environment/env-process.h"

void gen::SetupCore(wasm::Module& mod) {
	detail::MemoryBuilder _memory;
	detail::MappingBuilder _mapping;
	detail::ContextBuilder _context;
	detail::InteractBuilder _interact;

	/* initialize the core-imports */
	_memory.setupCoreImports(mod);
	_mapping.setupCoreImports(mod);

	/* setup the shared components */
	env::detail::ProcessAccess _proc = env::detail::ProcessAccess{};
	wasm::Memory physical = mod.memory(u8"memory_physical", wasm::Limit{ _proc.physicalPages() }, wasm::Export{});
	wasm::Memory management = mod.memory(u8"memory_management", wasm::Limit{ _proc.managementPages(), _proc.managementPages() }, wasm::Export{});

	/* setup the core-bodies */
	_memory.setupCoreBody(mod, management, physical);
	_mapping.setupCoreBody(mod, management);
	_context.setupCoreBody(mod, management);
}
