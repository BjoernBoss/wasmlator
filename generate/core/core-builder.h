/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2025 Bjoern Boss Henrichsen */
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
		void setupCoreImports(wasm::Memory& memory) const;
		void setupCoreBody(wasm::Memory& physical) const;
		void setupBlockImports(wasm::Memory& physical, wasm::Memory& memory) const;
	};
}
