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

	/* initialize the core-imports */
	_memory.setupCoreImports(mod);
	_mapping.setupCoreImports(mod);

	/* setup the shared components */
	uint32_t mpages = env::detail::ProcessAccess::ManagementPages(), ppages = env::detail::ProcessAccess::PhysicalPages();
	wasm::Memory physical = mod.memory(u8"memory_physical", wasm::Limit{ ppages }, wasm::Export{});
	wasm::Memory management = mod.memory(u8"memory_management", wasm::Limit{ mpages, mpages }, wasm::Export{});

	/* setup the core-bodies */
	_memory.setupCoreBody(mod, management, physical);
	_mapping.setupCoreBody(mod, management);
	_context.setupCoreBody(mod, management);
}
