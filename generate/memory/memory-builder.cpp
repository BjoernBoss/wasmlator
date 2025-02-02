#include "../generate.h"
#include "../environment/memory/env-memory.h"
#include "../environment/process/process-access.h"

void gen::detail::MemoryBuilder::fMakeLookup(const wasm::Memory& memory, const wasm::Function& function, size_t cache, size_t size, uint32_t usage) const {
	wasm::Sink sink{ function };

	/* perform the call (will automatically populate the cache properly) */
	sink[I::Param::Get(0)];
	sink[I::Param::Get(1)];
	sink[I::U32::Const(size)];
	sink[I::U32::Const(usage)];
	sink[I::U32::Const(cache)];
	sink[I::Call::Direct(pLookup)];

	/* compute the offset into the current cache-slot */
	sink[I::Param::Get(1)];
	sink[I::U32::Const(sizeof(env::detail::MemoryCache) * cache)];
	sink[I::U64::Load(memory, uint32_t(env::detail::MemoryAccess::CacheAddress()) + offsetof(env::detail::MemoryCache, address))];
	sink[I::U64::Sub()];

	/* add it to the actual physical address */
	sink[I::U64::Shrink()];
	sink[I::U32::Const(sizeof(env::detail::MemoryCache) * cache)];
	sink[I::U32::Load(memory, uint32_t(env::detail::MemoryAccess::CacheAddress()) + offsetof(env::detail::MemoryCache, physical))];
	sink[I::U32::Add()];
}

void gen::detail::MemoryBuilder::setupGlueMappings(detail::GlueState& glue) {
	glue.define(u8"mem_write_to_physical", { { u8"dest", wasm::Type::i32 }, { u8"source", wasm::Type::i32 }, { u8"size", wasm::Type::i32 } }, {});
	glue.define(u8"mem_read_from_physical", { { u8"dest", wasm::Type::i32 }, { u8"source", wasm::Type::i32 }, { u8"size", wasm::Type::i32 } }, {});
	glue.define(u8"mem_clear_physical", { { u8"dest", wasm::Type::i32 }, { u8"size", wasm::Type::i32 } }, {});
	glue.define(u8"mem_expand_physical", { { u8"pages", wasm::Type::i32 } }, { wasm::Type::i32 });
	glue.define(u8"mem_move_physical", { { u8"dest", wasm::Type::i32 }, { u8"source", wasm::Type::i32 }, { u8"size", wasm::Type::i32 } }, {});
	glue.define(u8"mem_read", { { u8"address", wasm::Type::i64 }, { u8"size", wasm::Type::i32 } }, { wasm::Type::i64 });
	glue.define(u8"mem_write", { { u8"address", wasm::Type::i64 }, { u8"size", wasm::Type::i32 }, { u8"value", wasm::Type::i64 } }, {});
	glue.define(u8"mem_code", { { u8"address", wasm::Type::i64 }, { u8"size", wasm::Type::i32 } }, { wasm::Type::i64 });
}
void gen::detail::MemoryBuilder::setupCoreImports() {
	/* add the import to the lookup-function */
	wasm::Prototype prototype = gen::Module->prototype(u8"main_lookup_type", { { u8"address", wasm::Type::i64 }, { u8"access", wasm::Type::i64 }, { u8"size", wasm::Type::i32 }, { u8"usage", wasm::Type::i32 }, { u8"cache", wasm::Type::i32 } }, {});
	pLookup = gen::Module->function(u8"main_lookup", prototype, wasm::Import{ u8"main" });

	/* define the bindings passed to the blocks */
	size_t caches = env::detail::MemoryAccess::CacheCount();
	for (size_t i = 0; i < caches * 4; ++i) {
		size_t cache = (i / 4), size = MemoryBuilder::Sizes[i % 4];
		env::detail::ProcessAccess::AddCoreBinding(u8"mem", str::u8::Build(u8"mem_lookup_read_", size, u8'_', cache));
		env::detail::ProcessAccess::AddCoreBinding(u8"mem", str::u8::Build(u8"mem_lookup_write_", size, u8'_', cache));
	}
}
void gen::detail::MemoryBuilder::setupCoreBody(const wasm::Memory& memory, const wasm::Memory& physical) const {
	detail::MemoryState state{ memory, physical };

	/* add the lookup-wrapper function, which performs the lookup and computes the final address */
	wasm::Prototype prototype = gen::Module->prototype(u8"mem_lookup_wrapper_type", { { u8"address", wasm::Type::i64 }, { u8"access", wasm::Type::i64 }, { u8"size", wasm::Type::i32 }, { u8"usage", wasm::Type::i32 }, { u8"cache", wasm::Type::i32 } }, { wasm::Type::i32 });
	wasm::Function lookupWrapper = gen::Module->function(u8"mem_lookup_wrapper", prototype, wasm::Export{});
	{
		wasm::Sink sink{ lookupWrapper };
		wasm::Variable cacheOffset = sink.local(wasm::Type::i32, u8"mem_lookup_cache");

		/* perform the call (will automatically populate the cache properly) */
		sink[I::Param::Get(0)];
		sink[I::Param::Get(1)];
		sink[I::Param::Get(2)];
		sink[I::Param::Get(3)];
		sink[I::Param::Get(4)];
		sink[I::Call::Direct(pLookup)];

		/* compute the offset into the current cache-slot */
		sink[I::Param::Get(1)];
		sink[I::Param::Get(4)];
		sink[I::U32::Const(sizeof(env::detail::MemoryCache))];
		sink[I::U32::Mul()];
		sink[I::Local::Tee(cacheOffset)];
		sink[I::U64::Load(memory, uint32_t(env::detail::MemoryAccess::CacheAddress()) + offsetof(env::detail::MemoryCache, address))];
		sink[I::U64::Sub()];

		/* add it to the actual physical address */
		sink[I::U64::Shrink()];
		sink[I::Local::Get(cacheOffset)];
		sink[I::U32::Load(memory, uint32_t(env::detail::MemoryAccess::CacheAddress()) + offsetof(env::detail::MemoryCache, physical))];
		sink[I::U32::Add()];
	}

	/* add the lookup functions */
	wasm::Function coreReadLookup[4] = {};
	wasm::Function coreWriteLookup[4] = {};
	wasm::Function coreCodeLookup[4] = {};
	{
		wasm::Prototype prototype = gen::Module->prototype(u8"mem_lookup_type", { { u8"address", wasm::Type::i64 }, { u8"access", wasm::Type::i64 } }, { wasm::Type::i32 });

		/* iteratively create the lookups for all size/cache-indices combinations */
		size_t caches = env::detail::MemoryAccess::CacheCount();
		for (size_t i = 0; i < caches * 4; ++i) {
			size_t cache = (i / 4), size = MemoryBuilder::Sizes[i % 4];

			/* create the read-function and add the implementation */
			wasm::Function read = gen::Module->function(str::u8::Build(u8"mem_lookup_read_", size, u8'_', cache), prototype, wasm::Export{});
			state.reads.push_back(read);
			fMakeLookup(memory, read, cache + env::detail::MemoryAccess::StartOfReadCaches(), size, env::Usage::Read);

			/* create the write-function and add the implementation */
			wasm::Function write = gen::Module->function(str::u8::Build(u8"mem_lookup_write_", size, u8'_', cache), prototype, wasm::Export{});
			state.writes.push_back(write);
			fMakeLookup(memory, write, cache + env::detail::MemoryAccess::StartOfWriteCaches(), size, env::Usage::Write);
		}

		/* create the core lookup-functions */
		for (size_t i = 0; i < 4; ++i) {
			size_t size = MemoryBuilder::Sizes[i];

			/* create the read-function */
			coreReadLookup[i] = gen::Module->function(str::u8::Build(u8"mem_core_lookup_read_", size), prototype);
			fMakeLookup(memory, coreReadLookup[i], env::detail::MemoryAccess::ReadCache(), size, env::Usage::Read);

			/* create the write-function */
			coreWriteLookup[i] = gen::Module->function(str::u8::Build(u8"mem_core_lookup_write_", size), prototype);
			fMakeLookup(memory, coreWriteLookup[i], env::detail::MemoryAccess::WriteCache(), size, env::Usage::Write);

			/* create the code-function */
			coreCodeLookup[i] = gen::Module->function(str::u8::Build(u8"mem_core_lookup_code_", size), prototype);
			fMakeLookup(memory, coreCodeLookup[i], env::detail::MemoryAccess::CodeCache(), size, env::Usage::Execute);
		}
	}

	/* add the write-to-physical function */
	{
		prototype = gen::Module->prototype(u8"mem_write_to_physical_type", { { u8"dest", wasm::Type::i32 }, { u8"source", wasm::Type::i32 }, { u8"size", wasm::Type::i32 } }, {});
		wasm::Sink sink{ gen::Module->function(u8"mem_write_to_physical", prototype, wasm::Export{}) };

		/* setup the copy operation and perform it */
		sink[I::Param::Get(0)];
		sink[I::Param::Get(1)];
		sink[I::Param::Get(2)];
		sink[I::Memory::Copy(physical, memory)];
	}

	/* add the read-from-physical function */
	{
		prototype = gen::Module->prototype(u8"mem_read_from_physical_type", { { u8"dest", wasm::Type::i32 }, { u8"source", wasm::Type::i32 }, { u8"size", wasm::Type::i32 } }, {});
		wasm::Sink sink{ gen::Module->function(u8"mem_read_from_physical", prototype, wasm::Export{}) };

		/* setup the copy operation and perform it */
		sink[I::Param::Get(0)];
		sink[I::Param::Get(1)];
		sink[I::Param::Get(2)];
		sink[I::Memory::Copy(memory, physical)];
	}

	/* add the clear-physical function */
	{
		prototype = gen::Module->prototype(u8"mem_clear_physical_type", { { u8"dest", wasm::Type::i32 }, { u8"size", wasm::Type::i32 } }, {});
		wasm::Sink sink{ gen::Module->function(u8"mem_clear_physical", prototype, wasm::Export{}) };

		/* setup the clear operation and perform it */
		sink[I::Param::Get(0)];
		sink[I::U32::Const(0)];
		sink[I::Param::Get(1)];
		sink[I::Memory::Fill(physical)];
	}

	/* add the memory-expansion function */
	{
		prototype = gen::Module->prototype(u8"mem_expand_physical_type", { { u8"pages", wasm::Type::i32 } }, { wasm::Type::i32 });
		wasm::Sink sink{ gen::Module->function(u8"mem_expand_physical", prototype, wasm::Export{}) };

		/* number of pages to grow by */
		sink[I::Param::Get(0)];

		sink[I::Memory::Grow(physical)];

		/* convert result to 1 or 0 */
		sink[I::I32::Const(0)];
		sink[I::I32::Less()];
		{
			wasm::IfThen _if{ sink, u8"", {}, {wasm::Type::i32 } };
			sink[I::U32::Const(0)];
			_if.otherwise();
			sink[I::U32::Const(1)];
		}
	}

	/* add the memory-move function */
	{
		wasm::Prototype prototype = gen::Module->prototype(u8"mem_move_physical_type", { { u8"dest", wasm::Type::i32 }, { u8"source", wasm::Type::i32 }, { u8"size", wasm::Type::i32 } }, {});
		wasm::Sink sink{ gen::Module->function(u8"mem_move_physical", prototype, wasm::Export{}) };

		/* destination-address */
		sink[I::Param::Get(0)];

		/* source-address */
		sink[I::Param::Get(1)];

		/* size */
		sink[I::Param::Get(2)];

		sink[I::Memory::Copy(physical)];
	}

	/* add the read-access function */
	{
		prototype = gen::Module->prototype(u8"mem_read", { { u8"address", wasm::Type::i64 }, { u8"size", wasm::Type::i32 } }, { wasm::Type::i64 });
		wasm::Sink sink{ gen::Module->function(u8"mem_read", prototype, wasm::Export{}) };

		/* bind the sink (necessary for the writer) */
		gen::Instance()->setSink(&sink);
		detail::MemoryWriter _writer{ state };

		sink[I::Param::Get(0)];
		wasm::Block _block8{ sink, u8"size_8", { wasm::Type::i64 }, { wasm::Type::i64 } };
		wasm::Block _block4{ sink, u8"size_4", { wasm::Type::i64 }, { wasm::Type::i64 } };
		wasm::Block _block2{ sink, u8"size_2", { wasm::Type::i64 }, { wasm::Type::i64 } };
		wasm::Block _block1{ sink, u8"size_1", { wasm::Type::i64 }, { wasm::Type::i64 } };
		sink[I::Param::Get(1)];
		sink[I::U32::TrailingNulls()];
		sink[I::Branch::Table({ _block1, _block2, _block4 }, _block8)];
		_block1.close();
		_writer.fMakeRead(env::detail::MemoryAccess::ReadCache(), gen::MemoryType::u8To64, env::detail::MainAccessAddress, coreReadLookup + 0);
		sink[I::Return()];

		_block2.close();
		_writer.fMakeRead(env::detail::MemoryAccess::ReadCache(), gen::MemoryType::u16To64, env::detail::MainAccessAddress, coreReadLookup + 1);
		sink[I::Return()];

		_block4.close();
		_writer.fMakeRead(env::detail::MemoryAccess::ReadCache(), gen::MemoryType::u32To64, env::detail::MainAccessAddress, coreReadLookup + 2);
		sink[I::Return()];

		_block8.close();
		_writer.fMakeRead(env::detail::MemoryAccess::ReadCache(), gen::MemoryType::i64, env::detail::MainAccessAddress, coreReadLookup + 3);

		/* clear the sink reference */
		gen::Instance()->setSink(0);
	}

	/* add the write-access function */
	{
		prototype = gen::Module->prototype(u8"mem_write", { { u8"address", wasm::Type::i64 }, { u8"size", wasm::Type::i32 }, { u8"value", wasm::Type::i64 } }, {});
		wasm::Sink sink{ gen::Module->function(u8"mem_write", prototype, wasm::Export{}) };

		/* bind the sink (necessary for the writer) */
		gen::Instance()->setSink(&sink);
		detail::MemoryWriter _writer{ state };

		sink[I::Param::Get(0)];
		wasm::Block _block8{ sink, u8"size_8", { wasm::Type::i64 }, { wasm::Type::i64 } };
		wasm::Block _block4{ sink, u8"size_4", { wasm::Type::i64 }, { wasm::Type::i64 } };
		wasm::Block _block2{ sink, u8"size_2", { wasm::Type::i64 }, { wasm::Type::i64 } };
		wasm::Block _block1{ sink, u8"size_1", { wasm::Type::i64 }, { wasm::Type::i64 } };
		sink[I::Param::Get(1)];
		sink[I::U32::TrailingNulls()];
		sink[I::Branch::Table({ _block1, _block2, _block4 }, _block8)];
		_block1.close();
		_writer.fMakeStartWrite(env::detail::MemoryAccess::WriteCache(), gen::MemoryType::u8To64, env::detail::MainAccessAddress, coreWriteLookup + 0);
		sink[I::Param::Get(2)];
		_writer.fMakeStopWrite(gen::MemoryType::u8To64);
		sink[I::Return()];

		_block2.close();
		_writer.fMakeStartWrite(env::detail::MemoryAccess::WriteCache(), gen::MemoryType::u16To64, env::detail::MainAccessAddress, coreWriteLookup + 1);
		sink[I::Param::Get(2)];
		_writer.fMakeStopWrite(gen::MemoryType::u16To64);
		sink[I::Return()];

		_block4.close();
		_writer.fMakeStartWrite(env::detail::MemoryAccess::WriteCache(), gen::MemoryType::u32To64, env::detail::MainAccessAddress, coreWriteLookup + 2);
		sink[I::Param::Get(2)];
		_writer.fMakeStopWrite(gen::MemoryType::u32To64);
		sink[I::Return()];

		_block8.close();
		_writer.fMakeStartWrite(env::detail::MemoryAccess::WriteCache(), gen::MemoryType::i64, env::detail::MainAccessAddress, coreWriteLookup + 3);
		sink[I::Param::Get(2)];
		_writer.fMakeStopWrite(gen::MemoryType::i64);

		/* clear the sink reference */
		gen::Instance()->setSink(0);
	}

	/* add the code-access function */
	{
		prototype = gen::Module->prototype(u8"mem_code", { { u8"address", wasm::Type::i64 }, { u8"size", wasm::Type::i32 } }, { wasm::Type::i64 });
		wasm::Sink sink{ gen::Module->function(u8"mem_code", prototype, wasm::Export{}) };

		/* bind the sink (necessary for the writer) */
		gen::Instance()->setSink(&sink);
		detail::MemoryWriter _writer{ state };

		sink[I::Param::Get(0)];
		wasm::Block _block8{ sink, u8"size_8", { wasm::Type::i64 }, { wasm::Type::i64 } };
		wasm::Block _block4{ sink, u8"size_4", { wasm::Type::i64 }, { wasm::Type::i64 } };
		wasm::Block _block2{ sink, u8"size_2", { wasm::Type::i64 }, { wasm::Type::i64 } };
		wasm::Block _block1{ sink, u8"size_1", { wasm::Type::i64 }, { wasm::Type::i64 } };
		sink[I::Param::Get(1)];
		sink[I::U32::TrailingNulls()];
		sink[I::Branch::Table({ _block1, _block2, _block4 }, _block8)];
		_block1.close();
		_writer.fMakeRead(env::detail::MemoryAccess::CodeCache(), gen::MemoryType::u8To64, env::detail::MainAccessAddress, coreCodeLookup + 0);
		sink[I::Return()];

		_block2.close();
		_writer.fMakeRead(env::detail::MemoryAccess::CodeCache(), gen::MemoryType::u16To64, env::detail::MainAccessAddress, coreCodeLookup + 1);
		sink[I::Return()];

		_block4.close();
		_writer.fMakeRead(env::detail::MemoryAccess::CodeCache(), gen::MemoryType::u32To64, env::detail::MainAccessAddress, coreCodeLookup + 2);
		sink[I::Return()];

		_block8.close();
		_writer.fMakeRead(env::detail::MemoryAccess::CodeCache(), gen::MemoryType::i64, env::detail::MainAccessAddress, coreCodeLookup + 3);

		/* clear the sink reference */
		gen::Instance()->setSink(0);
	}
}
void gen::detail::MemoryBuilder::setupBlockImports(const wasm::Memory& memory, const wasm::Memory& physical, detail::MemoryState& state) const {
	state.memory = memory;
	state.physical = physical;

	/* add the function-imports for the page-lookup */
	wasm::Prototype prototype = gen::Module->prototype(u8"mem_lookup_usage_type", { { u8"address", wasm::Type::i64 }, { u8"access", wasm::Type::i64 } }, { wasm::Type::i32 });

	/* define the bindings passed to the blocks */
	size_t caches = env::detail::MemoryAccess::CacheCount();
	for (size_t i = 0; i < caches * 4; ++i) {
		size_t cache = (i / 4), size = MemoryBuilder::Sizes[i % 4];
		state.reads.push_back(gen::Module->function(str::u8::Build(u8"mem_lookup_read_", size, u8'_', cache), prototype, wasm::Import{ u8"mem" }));
		state.writes.push_back(gen::Module->function(str::u8::Build(u8"mem_lookup_write_", size, u8'_', cache), prototype, wasm::Import{ u8"mem" }));
	}
}
