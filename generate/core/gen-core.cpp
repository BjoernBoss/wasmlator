#include "gen-core.h"
#include "core-builder.h"
#include "../memory/memory-builder.h"
#include "../mapping/mapping-builder.h"

gen::Core::Core(wasm::Module& mod) : pModule{ mod } {
	detail::MemoryBuilder _memory;
	detail::MappingBuilder _mapping;
	detail::CoreBuilder _core;
	wasm::Global lastInstance;
	wasm::Memory memory, physical;

	/* initialize the core-imports */
	_memory.setupCoreImports(mod);
	_mapping.setupCoreImports(mod);
	pInteract.setupCoreImports(mod);
	pProcess.setupCoreImports(mod);
	_core.setupCoreImports(mod, memory);

	/* setup the core-bodies */
	_core.setupCoreBody(mod, physical);
	_memory.setupCoreBody(mod, memory, physical);
	_mapping.setupCoreBody(mod, memory);
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
