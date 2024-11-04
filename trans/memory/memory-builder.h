#pragma once

#include "../trans-common.h"
#include "../../env/memory/env-memory.h"

namespace trans::detail {
	struct MemoryState {
		wasm::Function read;
		wasm::Function code;
		wasm::Function write;
		wasm::Memory management;
		wasm::Memory physical;
	};

	class MemoryBuilder {
	private:
		wasm::Function pLookup;
		wasm::Function pGetAddress;
		wasm::Function pGetPhysical;
		wasm::Function pGetSize;

	private:
		void fMakeLookup(const detail::MemoryState& state, const wasm::Function& function, uint32_t usage) const;
		void fMakeAccess(wasm::Module& mod, const detail::MemoryState& state, wasm::Type type, std::u8string_view name, trans::MemoryType memoryType) const;

	public:
		void setupCoreImports(wasm::Module& mod, detail::MemoryState& state);
		void setupCoreBody(wasm::Module& mod, detail::MemoryState& state) const;
		void setupBlockImports(wasm::Module& mod, detail::MemoryState& state) const;
	};
}
