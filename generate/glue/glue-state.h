/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025 Bjoern Boss Henrichsen */
#pragma once

#include "../gen-common.h"

namespace gen::detail {
	/*
	*	Imports:
	*		ext_func host.host_get_function(ext_ref instance, i32 name, i32 size, i32 main_memory);
	*		ext_ref host.host_get_export(ext_ref instance, i32 name, i32 size);
	*		ext_ref host.host_make_object();
	*		void host.host_set_member(ext_ref obj, i32 name, i32 size, ext_ref value);
	*
	*	Exports to Main:
	*		as defined by the builders
	*		i32 glue_setup_core_map();
	*		void glue_reset_core_map();
	*
	*	Exports to Core:
	*		ext_ref glue_get_export(i32 name, i32 size);
	*		ext_func glue_get_function(i32 name, i32 size);
	*		ext_ref glue_make_object();
	*		void glue_assign(ext_ref obj, i32 name, i32 size, ext_ref value);
	*		void glue_set_imports(ext_ref obj);
	*
	*	Exports to Host:
	*		uint8_t memory[...];
	*		void set_last_instance(ext_ref object);
	*		ext_ref get_imports();
	*/

	struct GlueState {
	private:
		struct LoadFunction {
			uint32_t name = 0;
			uint32_t size = 0;
		};

	private:
		wasm::Module& pModule;
		wasm::Table pFunctions;
		wasm::Memory pMemory;
		wasm::Function pGetFunction;
		wasm::Function pGetExport;
		wasm::Function pMakeObject;
		wasm::Function pSetMember;
		std::vector<LoadFunction> pLoaded;
		uint32_t pEndOfMemory = 0;

	public:
		GlueState(wasm::Module& mod);

	public:
		void setup();
		void define(std::u8string_view name, std::initializer_list<wasm::Param> params, std::initializer_list<wasm::Type> result);
		void complete();
	};
}
