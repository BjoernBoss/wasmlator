#pragma once

#include "../gen-common.h"
#include "../glue/glue-state.h"
#include "../environment/mapping/env-mapping.h"

namespace gen::detail {
	static constexpr uint32_t MinBlockList = 4;
	static constexpr uint32_t BlockListGrowth = 2;
	static constexpr uint32_t MinFunctionList = 32;
	static constexpr uint32_t FunctionListGrowth = 16;

	struct MappingState {
		wasm::Function lookup;
		wasm::Table functions;
	};

	/*
	*	Core-Imports:
	*		i32 main.main_resolve(i64 address);
	*		void main.main_flushed();
	*		void main.main_block_loaded(i32 succeeded);
	*		void host.host_load_block(i32 ptr, i32 size);
	*		ext_func host.host_get_export(ext_ref instance, i32 ptr, i32 size);
	*
	*	Core-Exports to Main:
	*		i32 map_load_block(i32 ptr, i32 size, i32 exports);
	*		i32 map_define(i32 name, i32 size, i64 address);
	*		void map_flush_blocks();
	*		i32 map_execute(i64 address);
	*
	*	Core-Exports to Host:
	*		void map_block_loaded(ext_ref instance);
	*
	*	Core-Exports to Body:
	*		func_ref map_functions[...];
	*		i32 map_lookup(i64 address);
	*
	*	Body-Imports:
	*		func_ref core.map_functions[...];
	*		i32 core.map_lookup(i64 address);
	*/

	class MappingBuilder {
	private:
		wasm::Function pResolve;
		wasm::Function pFlushed;
		wasm::Function pBlockLoaded;
		wasm::Function pLoadBlock;
		wasm::Function pGetExport;

	public:
		void setupGlueMappings(detail::GlueState& glue);
		void setupCoreImports(wasm::Module& mod);
		void setupCoreBody(wasm::Module& mod, const wasm::Memory& management) const;
		void setupBlockImports(wasm::Module& mod, detail::MappingState& state) const;
	};
}
