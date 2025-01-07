#include "../generate.h"

gen::Core::Core(wasm::Module& mod) {
	detail::MemoryBuilder _memory;
	detail::ContextBuilder _context;
	detail::MappingBuilder _mapping;
	detail::CoreBuilder _core;
	wasm::Global lastInstance;
	wasm::Memory memory, physical;

	/* bind the module to the current generator */
	gen::Instance()->setModule(&mod);

	/* initialize the core-imports */
	_memory.setupCoreImports();
	_mapping.setupCoreImports();
	_context.setupCoreImports();
	pInteract.setupCoreImports();
	pProcess.setupCoreImports();
	_core.setupCoreImports(memory);

	/* setup the core-bodies */
	_core.setupCoreBody(physical);
	_memory.setupCoreBody(memory, physical);
	_mapping.setupCoreBody(memory);
	pProcess.setupCoreBody();
}

bool gen::Core::close() {
	if (pClosed)
		return true;
	pClosed = true;

	/* finalize the components */
	pInteract.finalizeCoreBody();
	pProcess.finalizeCoreBody();

	/* notify the generator about the completed core */
	if (!detail::GeneratorAccess::CoreCreated())
		return false;

	/* unbind the module and close it (as a precaution) */
	gen::Instance()->setModule(0)->close();
	return true;
}
