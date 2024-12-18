#pragma once

#include "../gen-common.h"
#include "../glue/glue-state.h"

namespace gen::detail {
	struct ProcessState {
		wasm::Function terminate;
		wasm::Function notDecodable;
		wasm::Function notReachable;
		wasm::Function singleStep;
	};

	/*
	*	Core-Imports:
	*		ext_ref glue.glue_get_export(i32 name, i32 size);
	*		ext_ref glue.glue_make_object();
	*		void glue.glue_assign(ext_ref obj, i32 name, i32 size, ext_ref value);
	*		void glue.glue_set_imports(ext_ref obj);
	*		void main.main_terminate(i32 code, i64 address);
	*		void main.main_not_decodable(i64 address);
	*		void main.main_not_reachable(i64 address);
	*		void main.main_single_step(i64 address);
	*
	*	Core-Exports to Main:
	*		i32 proc_export(i32 name, i32 size, i32 index);
	*		void proc_block_imports_prepare();
	*		void proc_block_imports_next_member(i32 name, i32 size);
	*		void proc_block_imports_set_value(i32 name, i32 size, i32 index);
	*		void proc_block_imports_commit(i32 null);
	*
	*	Body-Imports:
	*		void proc.main_terminate(i32 code, i64 address);
	*		void proc.main_not_decodable(i64 address);
	*		void proc.main_not_reachable(i64 address);
	*		void proc.main_single_step(i64 address);
	*/

	class ProcessBuilder {
	private:
		wasm::Function pGetExport;
		wasm::Function pMakeObject;
		wasm::Function pAssign;
		wasm::Function pSetImports;
		wasm::Table pBindings;

	public:
		void setupGlueMappings(detail::GlueState& glue);
		void setupCoreImports(wasm::Module& mod);
		void setupCoreBody(wasm::Module& mod);
		void finalizeCoreBody(wasm::Module& mod) const;
		void setupBlockImports(wasm::Module& mod, detail::ProcessState& state) const;
	};
}
