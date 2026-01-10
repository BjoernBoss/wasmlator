/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025-2026 Bjoern Boss Henrichsen */
#pragma once

#include "../gen-common.h"
#include "../glue/glue-state.h"

namespace gen::detail {
	/*
	*	Core-Imports:
	*		ext_ref glue.glue_get_export(i32 name, i32 size);
	*		ext_ref glue.glue_make_object();
	*		void glue.glue_assign(ext_ref obj, i32 name, i32 size, ext_ref value);
	*		void glue.glue_set_imports(ext_ref obj);
	*
	*	Core-Exports to Main:
	*		i32 proc_export(i32 name, i32 size, i32 index);
	*		void proc_block_imports_prepare();
	*		void proc_block_imports_next_member(i32 name, i32 size);
	*		void proc_block_imports_set_value(i32 name, i32 size, i32 index);
	*		void proc_block_imports_commit(i32 null);
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
		void setupCoreImports();
		void setupCoreBody();
		void finalizeCoreBody() const;
	};
}
