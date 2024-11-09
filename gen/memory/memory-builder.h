#pragma once

#include "../gen-common.h"
#include "../glue/glue-state.h"
#include "../env/memory/env-memory.h"

namespace gen::detail {
	struct MemoryState {
		wasm::Function read;
		wasm::Function code;
		wasm::Function write;
		wasm::Memory management;
		wasm::Memory physical;
	};

	/*
	*	Core-Imports:
	*		void main.main_lookup(i64 address, i32 size, i32 usage);
	*		i64 main.main_result_address();
	*		i32 main.main_result_physical();
	*		i32 main.main_result_size();
	*
	*	Core-Exports to Main:
	*		void mem_flush_caches();
	*		i32 mem_expand_physical(i32 pages);
	*		void mem_move_physical(i32 dest, i32 source, i32 size);
	*		i64 mem_read(i64 address, i32 size);
	*		void mem_write(i64 address, i32 size, i64 value);
	*		i64 mem_code(i64 address, i32 size);
	*
	*	Core-Exports to Body:
	*		i32 mem_lookup_read(i64 address, i32 size, i32 cache);
	*		i32 mem_lookup_write(i64 address, i32 size, i32 cache);
	*		i32 mem_lookup_code(i64 address, i32 size, i32 cache);
	*
	*	Body-Imports:
	*		i32 core.lookup_read(i64 address, i32 size, i32 cache);
	*		i32 core.lookup_write(i64 address, i32 size, i32 cache);
	*		i32 core.lookup_code(i64 address, i32 size, i32 cache);
	*/

	class MemoryBuilder {
	private:
		wasm::Function pLookup;
		wasm::Function pGetAddress;
		wasm::Function pGetPhysical;
		wasm::Function pGetSize;

	private:
		void fMakeLookup(const wasm::Memory& management, const wasm::Function& function, uint32_t usage) const;

	public:
		void setupGlueMappings(detail::GlueState& glue);
		void setupCoreImports(wasm::Module& mod);
		void setupCoreBody(wasm::Module& mod, const wasm::Memory& management, const wasm::Memory& physical) const;
		void setupBlockImports(wasm::Module& mod, const wasm::Memory& management, const wasm::Memory& physical, detail::MemoryState& state) const;
	};
}
