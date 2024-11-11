#include "gen-glue.h"
#include "glue-state.h"
#include "../memory/memory-builder.h"
#include "../context/context-builder.h"
#include "../mapping/mapping-builder.h"
#include "../interact/interact-builder.h"

void gen::SetupGlue(wasm::Module& mod) {
	detail::GlueState generator{ mod };

	generator.setup();

	detail::MemoryBuilder{}.setupGlueMappings(generator);
	detail::ContextBuilder{}.setupGlueMappings(generator);
	detail::MappingBuilder{}.setupGlueMappings(generator);
	detail::InteractBuilder{}.setupGlueMappings(generator);

	generator.complete();
}
