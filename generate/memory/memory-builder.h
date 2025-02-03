#pragma once

#include "../gen-common.h"
#include "../glue/glue-state.h"

namespace gen::detail {
	struct MemoryState {
		wasm::Memory memory;
		wasm::Memory physical;
		wasm::Function reads[size_t(gen::MemoryType::_last)];
		wasm::Function writes[size_t(gen::MemoryType::_last)];
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
	*		$type mem_lookup_read_$type(i64 address, i64 access, i32 cache);
	*		void mem_lookup_write_$type(i64 address, i64 access, i32 cache, $type value);
	*
	*	Body-Imports:
	*		$type mem.mem_lookup_read_$type(i64 address, i64 access, i32 cache);
	*		void mem.mem_lookup_write_$type(i64 address, i64 access, i32 cache, $type value);
	*/

	class MemoryBuilder {
	private:
		static constexpr size_t Sizes[4] = { 1, 2, 4, 8 };

	private:
		wasm::Function pLookup;

	private:
		wasm::Function fMakeReadWrapper(const wasm::Memory& memory, const wasm::Memory& physical) const;
		wasm::Function fMakeWriteWrapper(const wasm::Memory& memory, const wasm::Memory& physical) const;
		wasm::Function fMakeReadLookup(uint32_t size, bool read, gen::MemoryType type, const wasm::Prototype& prototype, const wasm::Function& wrapper) const;
		wasm::Function fMakeWriteLookup(uint32_t size, gen::MemoryType type, const wasm::Prototype& prototype, const wasm::Function& wrapper) const;

	public:
		void setupGlueMappings(detail::GlueState& glue);
		void setupCoreImports();
		void setupCoreBody(const wasm::Memory& memory, const wasm::Memory& physical) const;
		void setupBlockImports(const wasm::Memory& memory, const wasm::Memory& physical, detail::MemoryState& state) const;
	};
}
