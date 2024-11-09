#pragma once

#include "../trans-common.h"

namespace trans::detail {
	/*
	*	Imports:
	*		ext_func host.get_export(i32 ptr, i32 size);
	*
	*	Exports to Main:
	*		as defined by mapping-builder/context-builder/memory-builder
	*		i32 setup_core_functions();
	*
	*	Exports to Host:
	*		uint8_t memory[...];
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
		wasm::Function pGetExport;
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
