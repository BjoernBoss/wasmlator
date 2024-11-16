#pragma once

#include "../gen-common.h"

namespace gen::detail {
	/*
	*	Core-Exports to Body:
	*		uint8_t memory_physical[...];
	*		uint8_t memory_management[...];
	*/

	class CoreBuilder {
	public:
		void setupCoreBody(wasm::Module& mod, wasm::Memory& physical, wasm::Memory& management) const;
		void setupBlockImports(wasm::Module& mod, wasm::Memory& physical, wasm::Memory& management) const;
	};
}
