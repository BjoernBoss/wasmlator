#include "gen-core.h"
#include "../memory/memory-builder.h"
#include "../context/context-builder.h"
#include "../mapping/mapping-builder.h"
#include "../environment/process/process-access.h"

gen::Core::Core(wasm::Module& mod) : pModule{ mod } {
	detail::MemoryBuilder _memory;
	detail::MappingBuilder _mapping;
	detail::ContextBuilder _context;
	wasm::Global lastInstance;

	/* initialize the core-imports */
	_memory.setupCoreImports(mod);
	_mapping.setupCoreImports(mod);
	_context.setupCoreImports(mod);
	pInteract.setupCoreImports(mod);
	pProcess.setupCoreImports(mod);

	/* setup the shared components */
	uint32_t mpages = env::detail::ProcessAccess::ManagementPages(), ppages = env::detail::ProcessAccess::PhysicalPages();
	wasm::Memory physical = mod.memory(u8"memory_physical", wasm::Limit{ ppages }, wasm::Export{});
	wasm::Memory management = mod.memory(u8"memory_management", wasm::Limit{ mpages, mpages }, wasm::Export{});

	/* setup the core-bodies */
	_memory.setupCoreBody(mod, management, physical);
	_mapping.setupCoreBody(mod, management);
	_context.setupCoreBody(mod, management);
	pProcess.setupCoreBody(mod);
}
gen::Core::~Core() noexcept(false) {
	/* as closing may throw exceptions, only perform automated
	*	closing, if no exception is currently being handled */
	if (std::uncaught_exceptions() == 0)
		fClose();
}

void gen::Core::fClose() {
	if (pClosed)
		return;
	pClosed = true;

	/* finalize the components */
	pInteract.finalizeCoreBody(pModule);
	pProcess.finalizeCoreBody(pModule);

	/* close the module (as a precaution) */
	pModule.close();
}

void gen::Core::close() {
	fClose();
}
