#pragma once

#include "../gen-common.h"
#include "../glue/glue-state.h"

namespace gen::detail {
	/*
	*	Core-Imports:
	*		ext_ref glue.glue_get_export(i32 name, i32 size);
	*
	*	Core-Exports to Main:
	*		i32 proc_export(i32 name, i32 size, i32 index);
	*/

	class ProcessBuilder {
	private:
		wasm::Function pGetExport;
		wasm::Table pBindings;

	public:
		void setupGlueMappings(detail::GlueState& glue);
		void setupCoreImports(wasm::Module& mod);
		void setupCoreBody(wasm::Module& mod);
		void finalizeCoreBody(wasm::Module& mod) const;
	};
}
