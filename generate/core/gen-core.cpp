#include "gen-core.h"
#include "core-builder.h"
#include "../memory/memory-builder.h"
#include "../context/context-builder.h"
#include "../mapping/mapping-builder.h"

gen::Core::Core(wasm::Module& mod) : pModule{ mod } {
	detail::MemoryBuilder _memory;
	detail::MappingBuilder _mapping;
	detail::ContextBuilder _context;
	detail::CoreBuilder _core;
	wasm::Global lastInstance;

	/* initialize the core-imports */
	_memory.setupCoreImports(mod);
	_mapping.setupCoreImports(mod);
	_context.setupCoreImports(mod);
	pInteract.setupCoreImports(mod);
	pProcess.setupCoreImports(mod);

	/* setup the core-bodies */
	wasm::Memory physical, management;
	_core.setupCoreBody(mod, physical, management);
	_memory.setupCoreBody(mod, management, physical);
	_mapping.setupCoreBody(mod, management);
	_context.setupCoreBody(mod, management);
	pProcess.setupCoreBody(mod);
}
gen::Core::~Core() noexcept(false) {
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
