#include "process-builder.h"
#include "../environment/process/process-access.h"

namespace I = wasm::inst;

void gen::detail::ProcessBuilder::setupGlueMappings(detail::GlueState& glue) {
	glue.define(u8"proc_export", { { u8"name", wasm::Type::i32 }, { u8"size", wasm::Type::i32 }, { u8"index", wasm::Type::i32 } }, { wasm::Type::i32 });
	glue.define(u8"proc_block_imports_prepare", {}, {});
	glue.define(u8"proc_block_imports_next_member", { { u8"name", wasm::Type::i32 }, { u8"size", wasm::Type::i32 } }, {});
	glue.define(u8"proc_block_imports_set_value", { { u8"name", wasm::Type::i32 }, { u8"size", wasm::Type::i32 }, { u8"index", wasm::Type::i32 } }, {});
	glue.define(u8"proc_block_imports_commit", { { u8"null", wasm::Type::i32 } }, {});
}
void gen::detail::ProcessBuilder::setupCoreImports(wasm::Module& mod) {
	/* import the get-export function */
	wasm::Prototype prototype = mod.prototype(u8"glue_get_export_type", { { u8"name", wasm::Type::i32 }, { u8"size", wasm::Type::i32 } }, { wasm::Type::refExtern });
	pGetExport = mod.function(u8"glue_get_export", prototype, wasm::Import{ u8"glue" });

	/* import the make-object function */
	prototype = mod.prototype(u8"glue_make_object_type", {}, { wasm::Type::refExtern });
	pMakeObject = mod.function(u8"glue_make_object", prototype, wasm::Import{ u8"glue" });

	/* import the assign function */
	prototype = mod.prototype(u8"glue_assign_type", { { u8"obj", wasm::Type::refExtern }, { u8"name", wasm::Type::i32 }, { u8"size", wasm::Type::i32 }, { u8"value", wasm::Type::refExtern } }, {});
	pAssign = mod.function(u8"glue_assign", prototype, wasm::Import{ u8"glue" });

	/* import the set-imports function */
	prototype = mod.prototype(u8"glue_set_imports_type", { { u8"obj", wasm::Type::refExtern } }, {});
	pSetImports = mod.function(u8"glue_set_imports", prototype, wasm::Import{ u8"glue" });
}
void gen::detail::ProcessBuilder::setupCoreBody(wasm::Module& mod) {
	/* reserve bindings-table (dont set its limit yet) */
	pBindings = mod.table(u8"proc_bindings", false);

	/* allocate the two caches for the import-object construction */
	wasm::Global impObject = mod.global(u8"imports_object", wasm::Type::refExtern, true);
	mod.value(impObject, wasm::Value::MakeExtern());
	wasm::Global impCurrent = mod.global(u8"imports_module", wasm::Type::refExtern, true);
	mod.value(impCurrent, wasm::Value::MakeExtern());

	/* add the proc-export function */
	{
		wasm::Prototype prototype = mod.prototype(u8"proc_export_type", { { u8"name", wasm::Type::i32 }, { u8"size", wasm::Type::i32 }, { u8"index", wasm::Type::i32 } }, { wasm::Type::i32 });
		wasm::Sink sink{ mod.function(u8"proc_export", prototype, wasm::Export{}) };
		wasm::Variable ref = sink.local(wasm::Type::refExtern, u8"exported_reference");

		/* perform the lookup call */
		sink[I::Param::Get(0)];
		sink[I::Param::Get(1)];
		sink[I::Call::Direct(pGetExport)];
		sink[I::Local::Tee(ref)];

		/* check if the import failed */
		sink[I::Ref::IsNull()];
		{
			/* notify the application about the failure by returning 0 */
			wasm::IfThen _if{ sink };
			sink[I::U32::Const(0)];
			sink[I::Return()];
		}

		/* write the reference to the table */
		sink[I::Param::Get(2)];
		sink[I::Local::Get(ref)];
		sink[I::Table::Set(pBindings)];

		/* return 1 to indicate successful loading */
		sink[I::U32::Const(1)];
	}

	/* add the proc-prepare import function */
	{
		wasm::Prototype prototype = mod.prototype(u8"proc_block_imports_prepare_type", {}, {});
		wasm::Sink sink{ mod.function(u8"proc_block_imports_prepare", prototype, wasm::Export{}) };

		/* create the new object and write it to the cached entry */
		sink[I::Call::Direct(pMakeObject)];
		sink[I::Global::Set(impObject)];
	}

	/* add the proc-next-member import function */
	{
		wasm::Prototype prototype = mod.prototype(u8"proc_block_imports_next_member_type", { { u8"name", wasm::Type::i32 }, { u8"size", wasm::Type::i32 } }, {});
		wasm::Sink sink{ mod.function(u8"proc_block_imports_next_member", prototype, wasm::Export{}) };

		/* create the next object and write it to the cached entry */
		sink[I::Call::Direct(pMakeObject)];
		sink[I::Global::Set(impCurrent)];

		/* assign the object to the total import object */
		sink[I::Global::Get(impObject)];
		sink[I::Param::Get(0)];
		sink[I::Param::Get(1)];
		sink[I::Global::Get(impCurrent)];
		sink[I::Call::Tail(pAssign)];
	}

	/* add the proc-set-value import function */
	{
		wasm::Prototype prototype = mod.prototype(u8"proc_block_imports_set_value_type", { { u8"name", wasm::Type::i32 }, { u8"size", wasm::Type::i32 }, { u8"index", wasm::Type::i32 } }, {});
		wasm::Sink sink{ mod.function(u8"proc_block_imports_set_value", prototype, wasm::Export{}) };

		/* prepare the arguments for the call */
		sink[I::Global::Get(impCurrent)];
		sink[I::Param::Get(0)];
		sink[I::Param::Get(1)];

		/* fetch the value at the binding-slot index */
		sink[I::Param::Get(2)];
		sink[I::Table::Get(pBindings)];

		/* perform the final assign call */
		sink[I::Call::Tail(pAssign)];
	}

	/* add the proc-commit import function */
	{
		wasm::Prototype prototype = mod.prototype(u8"proc_block_imports_commit_type", { { u8"null", wasm::Type::i32 } }, {});
		wasm::Sink sink{ mod.function(u8"proc_block_imports_commit", prototype, wasm::Export{}) };

		/* pass the proper object to the glue-module */
		sink[I::Ref::NullExtern()];
		sink[I::Global::Get(impObject)];
		sink[I::Param::Get(0)];
		sink[I::Select(wasm::Type::refExtern)];
		sink[I::Call::Direct(pSetImports)];

		/* clear the local references to the objects (to allow garbage-collection to occur) */
		sink[I::Ref::NullExtern()];
		sink[I::Global::Set(impObject)];
		sink[I::Ref::NullExtern()];
		sink[I::Global::Set(impCurrent)];
	}
}
void gen::detail::ProcessBuilder::finalizeCoreBody(wasm::Module& mod) const {
	/* lock the bindings to prevent further bindings from being added */
	env::detail::ProcessAccess::LockBindings();

	/* finalize the limit of the bindings-table */
	uint32_t count = uint32_t(env::detail::ProcessAccess::BindingCount());
	mod.limit(pBindings, wasm::Limit{ count , count });
}
