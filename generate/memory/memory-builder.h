#pragma once

#include "../gen-common.h"
#include "../glue/glue-state.h"

namespace gen::detail {
	struct MemoryState {
		wasm::Memory memory;
		wasm::Memory physical;

		/* size * num-caches (size => (1, 2, 4, 8)) */
		std::vector<wasm::Function> reads;

		/* size * num-caches (size => (1, 2, 4, 8)) */
		std::vector<wasm::Function> writes;
	};

	/*
	*	Core-Imports:
	*		void main.main_lookup(i64 address, i64 access, i32 size, i32 usage, i32 cache);
	*
	*	Core-Exports to Main:
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
	*		i32 mem_lookup_read_$size_$cache(i64 address, i64 access);
	*		i32 mem_lookup_write_$size_$cache(i64 address, i64 access);
	*
	*	Body-Imports:
	*		i32 mem.mem_lookup_$size_$cache(i64 address, i64 access);
	*		i32 mem.mem_lookup_$size_$cache(i64 address, i64 access);
	*/

	class MemoryBuilder {
	private:
		static constexpr size_t Sizes[4] = { 1, 2, 4, 8 };

	private:
		wasm::Function pLookup;

	private:
		void fMakeLookup(const wasm::Memory& memory, const wasm::Function& function, size_t cache, size_t size, uint32_t usage) const;

	public:
		void setupGlueMappings(detail::GlueState& glue);
		void setupCoreImports();
		void setupCoreBody(const wasm::Memory& memory, const wasm::Memory& physical) const;
		void setupBlockImports(const wasm::Memory& memory, const wasm::Memory& physical, detail::MemoryState& state) const;
	};
}
