#include "../generate.h"
#include "../memory/memory-builder.h"
#include "../context/context-builder.h"
#include "../mapping/mapping-builder.h"

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
gen::Core::~Core() noexcept(false) {
	if (std::uncaught_exceptions() == 0)
		fClose();
}

void gen::Core::fClose() {
	if (pClosed)
		return;
	pClosed = true;

	/* finalize the components */
	pInteract.finalizeCoreBody();
	pProcess.finalizeCoreBody();

	/* unbind the module and close it (as a precaution) */
	wasm::Module& mod = gen::Instance()->_module();
	gen::Instance()->setModule(0);
	mod.close();
}

void gen::Core::close() {
	fClose();
}
