#include "interact-builder.h"
#include "../environment/process/process-access.h"
#include "../environment/interact/interact-access.h"

namespace I = wasm::inst;

void gen::detail::InteractBuilder::setupGlueMappings(detail::GlueState& glue) {
	glue.define(u8"int_call_void", { { u8"index", wasm::Type::i32 } }, {});
	glue.define(u8"int_call_param", { { u8"param", wasm::Type::i64 }, { u8"index", wasm::Type::i32 } }, { wasm::Type::i64 });
}
void gen::detail::InteractBuilder::setupCoreImports(wasm::Module& mod) const {
	/* import the main invoke-void method and pass it through as binding to the blocks */
	wasm::Prototype prototype = mod.prototype(u8"main_invoke_void_type", { { u8"index", wasm::Type::i32 } }, {});
	mod.function(u8"main_invoke_void", prototype, wasm::Transport{ u8"main" });
	env::detail::ProcessAccess::AddCoreBinding(u8"int", u8"main_invoke_void");

	/* import the main invoke-param method and pass it through as binding to the blocks */
	prototype = mod.prototype(u8"main_invoke_param_type", { { u8"param", wasm::Type::i64 }, { u8"index", wasm::Type::i32 } }, { wasm::Type::i64 });
	mod.function(u8"main_invoke_param", prototype, wasm::Transport{ u8"main" });
	env::detail::ProcessAccess::AddCoreBinding(u8"int", u8"main_invoke_param");
}
void gen::detail::InteractBuilder::finalizeCoreBody(wasm::Module& mod) const {
	/* add the exported function-table */
	wasm::Table exported = mod.table(u8"call_list", true);
	std::vector<wasm::Value> list;

	/* iterate over the defined functions and write the valid functions to the table */
	for (const wasm::Function& fn : mod.functions()) {
		wasm::Prototype proto = fn.prototype();
		if (fn.id().empty())
			continue;

		/* check if this is a void-function */
		if (proto.parameter().empty() && proto.result().empty()) {
			env::detail::InteractAccess::DefineExported(std::u8string{ fn.id() }, uint32_t(list.size()), false);
			list.push_back(wasm::Value::MakeFunction(fn));
		}

		/* check if this is a param-function */
		if (proto.parameter().size() == 1 && proto.parameter()[0].type == wasm::Type::i64 &&
			proto.result().size() == 1 && proto.result()[0] == wasm::Type::i64) {
			env::detail::InteractAccess::DefineExported(std::u8string{ fn.id() }, uint32_t(list.size()), true);
			list.push_back(wasm::Value::MakeFunction(fn));
		}
	}

	/* finalize the function-table */
	mod.limit(exported, wasm::Limit{ uint32_t(list.size()), uint32_t(list.size()) });
	mod.elements(exported, wasm::Value::MakeU32(0), list);

	/* add the void-call function */
	{
		wasm::Prototype prototype = mod.prototype(u8"int_call_void_type", { { u8"index", wasm::Type::i32 } }, {});
		wasm::Prototype voidFnType = mod.prototype(u8"int_void_fn_type", {}, {});
		wasm::Sink sink{ mod.function(u8"int_call_void", prototype, wasm::Export{}) };

		/* call the function at the given index in the table */
		sink[I::Param::Get(0)];
		sink[I::Call::IndirectTail(exported, voidFnType)];
	}

	/* add the param-call function */
	{
		wasm::Prototype prototype = mod.prototype(u8"int_call_param_type", { { u8"param", wasm::Type::i64 }, { u8"index", wasm::Type::i32 } }, { wasm::Type::i64 });
		wasm::Prototype paramFnType = mod.prototype(u8"int_param_fn_type", { { u8"param", wasm::Type::i64 } }, { wasm::Type::i64 });
		wasm::Sink sink{ mod.function(u8"int_call_param", prototype, wasm::Export{}) };

		/* call the function at the given index in the table */
		sink[I::Param::Get(0)];
		sink[I::Param::Get(1)];
		sink[I::Call::IndirectTail(exported, paramFnType)];
	}
}
void gen::detail::InteractBuilder::setupBlockImports(wasm::Module& mod, detail::InteractState& state) const {
	/* add the function-import for the void-invoke function */
	wasm::Prototype prototype = mod.prototype(u8"main_invoke_void_type", { { u8"index", wasm::Type::i32 } }, {});
	state.invokeVoid = mod.function(u8"main_invoke_void", prototype, wasm::Import{ u8"int" });

	/* add the function-import for the param-invoke function */
	prototype = mod.prototype(u8"main_invoke_param_type", { { u8"param", wasm::Type::i64 }, { u8"index", wasm::Type::i32 } }, { wasm::Type::i64 });
	state.invokeParam = mod.function(u8"main_invoke_param", prototype, wasm::Import{ u8"int" });
}
