#pragma once

#include "../gen-common.h"

namespace gen::detail {
	/*
	*	Core-Imports:
	*		uint8_t main.memory[...];
	*
	*	Core-Exports to Body:
	*		uint8_t memory_physical[...];
	*/

	class CoreBuilder {
	public:
		void setupCoreImports(wasm::Module& mod, wasm::Memory& memory) const;
		void setupCoreBody(wasm::Module& mod, wasm::Memory& physical) const;
		void setupBlockImports(wasm::Module& mod, wasm::Memory& physical, wasm::Memory& memory) const;
	};
}
