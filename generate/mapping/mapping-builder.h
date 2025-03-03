/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025 Bjoern Boss Henrichsen */
#pragma once

#include "../gen-common.h"
#include "../glue/glue-state.h"

namespace gen::detail {
	static constexpr uint32_t MinFunctionList = 32;
	static constexpr uint32_t FunctionListGrowth = 16;

	struct MappingState {
		wasm::Function lookup;
		wasm::Table functions;
		wasm::Prototype blockPrototype;
	};

	/*
	*	Core-Imports:
	*		i32 main.map_resolve(i64 address);
	*		ext_func glue.glue_get_function(i32 name, i32 size);
	*
	*	Core-Exports to Main:
	*		i32 map_reserve(i32 exports);
	*		i32 map_define(i32 name, i32 size, i64 address);
	*		void map_execute(i64 address);
	*		void map_flush();
	*
	*	Core-Exports to Body:
	*		ext_func map_functions[...];
	*		i32 map_lookup(i64 address);
	*
	*	Body-Imports:
	*		ext_func map.map_functions[...];
	*		i32 map.map_lookup(i64 address);
	*/

	class MappingBuilder {
	private:
		wasm::Function pResolve;
		wasm::Function pGetFunction;
		wasm::Prototype pBlockPrototype;

	public:
		void setupGlueMappings(detail::GlueState& glue);
		void setupCoreImports();
		void setupCoreBody(const wasm::Memory& memory) const;
		void setupBlockImports(wasm::Prototype& blockPrototype, detail::MappingState& state) const;
	};
}
