#pragma once

#include "../trans-common.h"
#include "../../env/mapping/env-mapping.h"

namespace trans::detail {
	static constexpr uint32_t MinBlockList = 4;
	static constexpr uint32_t BlockListGrowth = 2;
	static constexpr uint32_t MinFunctionList = 32;
	static constexpr uint32_t FunctionListGrowth = 16;

	/* must be zero, as wasm::EqualZero is used internally */
	static constexpr uint32_t NotFoundIndex = 0;

	struct MappingState {
		wasm::Function lookup;
		wasm::Table functions;
		wasm::Memory management;
	};

	class MappingBuilder {
	private:
		wasm::Function pResolve;
		wasm::Function pFlushed;
		wasm::Function pAssociate;

	public:
		void setupCoreImports(wasm::Module& mod, detail::MappingState& state);
		void setupCoreBody(wasm::Module& mod, detail::MappingState& state) const;
		void setupBlockImports(wasm::Module& mod, detail::MappingState& state) const;
	};
}
