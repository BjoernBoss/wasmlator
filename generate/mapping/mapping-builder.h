#pragma once

#include "../gen-common.h"
#include "../glue/glue-state.h"

namespace gen::detail {
	static constexpr uint32_t MinFunctionList = 32;
	static constexpr uint32_t FunctionListGrowth = 16;

	struct MappingState {
		wasm::Function lookup;
		wasm::Table functions;
	};

	/*
	*	Core-Imports:
	*		i32 main.map_resolve(i64 address);
	*		void main.map_flushed();
	*		ext_func glue.glue_get_function(i32 name, i32 size);
	*
	*	Core-Exports to Main:
	*		i32 map_reserve(i32 exports);
	*		i32 map_define(i32 name, i32 size, i64 address);
	*		void map_flush_blocks();
	*		i32 map_execute(i64 address);
	*
	*	Core-Exports to Body:
	*		func_ref map_functions[...];
	*		i32 map_lookup(i64 address);
	*
	*	Body-Imports:
	*		func_ref map.map_functions[...];
	*		i32 map.map_lookup(i64 address);
	*/

	class MappingBuilder {
	private:
		wasm::Function pResolve;
		wasm::Function pFlushed;
		wasm::Function pGetFunction;

	public:
		void setupGlueMappings(detail::GlueState& glue);
		void setupCoreImports(wasm::Module& mod);
		void setupCoreBody(wasm::Module& mod, const wasm::Memory& management) const;
		void setupBlockImports(wasm::Module& mod, detail::MappingState& state) const;
	};
}
