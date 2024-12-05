#pragma once

#include "../gen-common.h"
#include "../glue/glue-state.h"

namespace gen::detail {
	struct MemoryState {
		wasm::Function read;
		wasm::Function code;
		wasm::Function write;
		wasm::Memory memory;
		wasm::Memory physical;
	};

	/*
	*	Core-Imports:
	*		void main.main_lookup(i64 address, i64 access, i32 size, i32 usage, i32 cache);
	*
	*	Core-Exports to Main:
	*		void mem_flush_caches();
	*		void mem_write_to_physical(i32 dest, i32 source, i32 size);
	*		void mem_read_from_physical(i32 dest, i32 source, i32 size);
	*		void mem_clear_physical(i32 dest, i32 size);
	*		i32 mem_expand_physical(i32 pages);
	*		void mem_move_physical(i32 dest, i32 source, i32 size);
	*		i64 mem_read(i64 address, i32 size);
	*		void mem_write(i64 address, i32 size, i64 value);
	*		i64 mem_code(i64 address, i32 size);
	*
	*	Core-Exports to Body:
	*		i32 mem_lookup_read(i64 address, i64 access, i32 size, i32 cache);
	*		i32 mem_lookup_write(i64 address, i64 access, i32 size, i32 cache);
	*		i32 mem_lookup_code(i64 address, i64 access, i32 size, i32 cache);
	*
	*	Body-Imports:
	*		i32 mem.mem_lookup_read(i64 address, i64 access, i32 size, i32 cache);
	*		i32 mem.mem_lookup_write(i64 address, i64 access, i32 size, i32 cache);
	*		i32 mem.mem_lookup_code(i64 address, i64 access, i32 size, i32 cache);
	*/

	class MemoryBuilder {
	private:
		wasm::Function pLookup;

	private:
		void fMakeLookup(const wasm::Memory& memory, const wasm::Function& function, uint32_t usage) const;

	public:
		void setupGlueMappings(detail::GlueState& glue);
		void setupCoreImports(wasm::Module& mod);
		void setupCoreBody(wasm::Module& mod, const wasm::Memory& memory, const wasm::Memory& physical) const;
		void setupBlockImports(wasm::Module& mod, const wasm::Memory& memory, const wasm::Memory& physical, detail::MemoryState& state) const;
	};
}
