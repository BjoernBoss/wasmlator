#include "process-builder.h"
#include "../environment/process/process-access.h"

namespace I = wasm::inst;

void gen::detail::ProcessBuilder::setupGlueMappings(detail::GlueState& glue) {
	glue.define(u8"proc_export", { { u8"name", wasm::Type::i32 }, { u8"size", wasm::Type::i32 }, { u8"index", wasm::Type::i32 } }, { wasm::Type::i32 });
}
void gen::detail::ProcessBuilder::setupCoreImports(wasm::Module& mod) {
	/* import the get-export function */
	wasm::Prototype prototype = mod.prototype(u8"glue_get_export_type", { { u8"name", wasm::Type::i32 }, { u8"size", wasm::Type::i32 } }, { wasm::Type::refExtern });
	pGetExport = mod.function(u8"glue_get_export", prototype, wasm::Import{ u8"glue" });
}
void gen::detail::ProcessBuilder::setupCoreBody(wasm::Module& mod) {
	/* reserve bindings-table (dont set its limit yet) */
	pBindings = mod.table(u8"proc_bindings", false);

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
}
void gen::detail::ProcessBuilder::finalizeCoreBody(wasm::Module& mod) const {
	/* finalize the limit of the bindings-table */
	uint32_t count = uint32_t(env::detail::ProcessAccess::BindingCount());
	mod.limit(pBindings, wasm::Limit{ count , count });
}
