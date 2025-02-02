#include "../generate.h"
#include "../environment/memory/env-memory.h"
#include "../environment/process/process-access.h"

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

	/* add the read-lookup-wrapper function, which performs the lookup and returns the actual value (will leave
	*	the cache at the last-most address - meaning another contiguous read will have a valid cache) */
	wasm::Prototype prototype = gen::Module->prototype(u8"mem_wrapper_read_type", { { u8"address", wasm::Type::i64 }, { u8"access", wasm::Type::i64 },
		{ u8"size", wasm::Type::i32 }, { u8"usage", wasm::Type::i32 }, { u8"cache", wasm::Type::i32 } }, { wasm::Type::i64 });
	wasm::Function readLookupWrapper = gen::Module->function(u8"mem_wrapper_read", prototype, wasm::Export{});
	{
		wasm::Sink sink{ readLookupWrapper };
		wasm::Variable cacheOffset = sink.local(wasm::Type::i32, u8"mem_lookup_cache");
		wasm::Variable offset = sink.local(wasm::Type::i32, u8"mem_lookup_offset");
		wasm::Variable size = sink.local(wasm::Type::i32, u8"mem_lookup_total");
		wasm::Variable actual = sink.local(wasm::Type::i32, u8"mem_lookup_physical");
		wasm::Variable index = sink.local(wasm::Type::i32, u8"mem_lookup_index");
		wasm::Variable value = sink.local(wasm::Type::i64, u8"mem_lookup_value");

		/* compute the base cache address */
		sink[I::Param::Get(4)];
		sink[I::U32::Const(sizeof(env::detail::MemoryCache))];
		sink[I::U32::Mul()];
		sink[I::Local::Set(cacheOffset)];

		/* perform the call for the base address (will automatically populate the cache properly) */
		sink[I::Param::Get(0)];
		sink[I::Param::Get(1)];
		sink[I::Param::Get(2)];
		sink[I::Param::Get(3)];
		sink[I::Param::Get(4)];
		sink[I::Call::Direct(pLookup)];

		/* compute the offset of the accessed address into the current cache entry */
		sink[I::Param::Get(1)];
		sink[I::Local::Get(cacheOffset)];
		sink[I::U64::Load(memory, uint32_t(env::detail::MemoryAccess::CacheAddress()) + offsetof(env::detail::MemoryCache, address))];
		sink[I::U64::Sub()];
		sink[I::U64::Shrink()];
		sink[I::Local::Tee(offset)];

		/* compute the physical address */
		sink[I::Local::Get(cacheOffset)];
		sink[I::U32::Load(memory, uint32_t(env::detail::MemoryAccess::CacheAddress()) + offsetof(env::detail::MemoryCache, physical))];
		sink[I::U32::Add()];
		sink[I::Local::Tee(actual)];

		/* check if the entire value can be read form the single cache (lower bound will match, assured by lookup) */
		sink[I::Local::Get(offset)];
		sink[I::Param::Get(2)];
		sink[I::U32::Add()];
		sink[I::Local::Get(cacheOffset)];
		sink[I::U32::Load(memory, uint32_t(env::detail::MemoryAccess::CacheAddress()) + offsetof(env::detail::MemoryCache, total))];
		sink[I::Local::Tee(size)];
		sink[I::U32::LessEqual()];

		/* the entire values lies in the single cache-entry (default case) */
		{
			wasm::IfThen _if{ sink, u8"", { wasm::Type::i32 }, { wasm::Type::i32 } };

			/* perform the sized read */
			wasm::Block _block8{ sink, u8"size_8", { wasm::Type::i32 }, { wasm::Type::i32 } };
			wasm::Block _block4{ sink, u8"size_4", { wasm::Type::i32 }, { wasm::Type::i32 } };
			wasm::Block _block2{ sink, u8"size_2", { wasm::Type::i32 }, { wasm::Type::i32 } };
			wasm::Block _block1{ sink, u8"size_1", { wasm::Type::i32 }, { wasm::Type::i32 } };
			sink[I::Param::Get(2)];
			sink[I::U32::TrailingNulls()];
			sink[I::Branch::Table({ _block1, _block2, _block4 }, _block8)];

			/* add the size-1 read */
			_block1.close();
			sink[I::U64::Load8(physical)];
			sink[I::Return()];

			/* add the size-2 read */
			_block2.close();
			sink[I::U64::Load16(physical)];
			sink[I::Return()];

			/* add the size-4 read */
			_block4.close();
			sink[I::U64::Load32(physical)];
			sink[I::Return()];

			/* add the size-8 read */
			_block8.close();
			sink[I::U64::Load(physical)];
			sink[I::Return()];
		}

		/* update the size counter to contain the number of available bytes */
		sink[I::Local::Get(size)];
		sink[I::Local::Get(offset)];
		sink[I::U32::Sub()];
		sink[I::Local::Set(size)];

		/* read the value byte-wise */
		{
			wasm::Loop _loop{ sink, u8"", { wasm::Type::i32 }, {} };

			/* read the next byte and write it to the value */
			sink[I::U64::Load8(physical, 0)];
			sink[I::Local::Get(index)];
			sink[I::U32::Const(8)];
			sink[I::U32::Mul()];
			sink[I::U32::Expand()];
			sink[I::U64::ShiftLeft()];

			/* add the byte to the output value */
			sink[I::Local::Get(value)];
			sink[I::U64::Or()];
			sink[I::Local::Set(value)];

			/* advance the index */
			sink[I::U32::Const(1)];
			sink[I::Local::Get(index)];
			sink[I::U32::Add()];
			sink[I::Local::Tee(index)];

			/* check if the end has been reached */
			sink[I::Param::Get(2)];
			sink[I::U32::GreaterEqual()];
			{
				wasm::IfThen _if{ sink };
				sink[I::Local::Get(value)];
				sink[I::Return()];
			}

			/* check if the next byte still lies in the current cache entry */
			sink[I::Local::Get(index)];
			sink[I::Local::Get(size)];
			sink[I::U32::Less()];
			{
				wasm::IfThen _if{ sink };

				/* advance the physical address and write it to the stack */
				sink[I::Local::Get(actual)];
				sink[I::U32::Const(1)];
				sink[I::U32::Add()];
				sink[I::Local::Tee(actual)];
				sink[I::Branch::Direct(_loop)];
			}

			/* perform the next cache lookup */
			sink[I::Param::Get(0)];
			sink[I::Param::Get(1)];
			sink[I::Local::Get(index)];
			sink[I::U32::Expand()];
			sink[I::U64::Add()];
			sink[I::Param::Get(2)];
			sink[I::Param::Get(3)];
			sink[I::Param::Get(4)];
			sink[I::Call::Direct(pLookup)];

			/* fetch the new remaining total-counter */
			sink[I::Local::Get(cacheOffset)];
			sink[I::U32::Load(memory, uint32_t(env::detail::MemoryAccess::CacheAddress()) + offsetof(env::detail::MemoryCache, total))];
			sink[I::Local::Set(size)];

			/* fetch the new actual address and leave it on the stack */
			sink[I::Local::Get(cacheOffset)];
			sink[I::U32::Load(memory, uint32_t(env::detail::MemoryAccess::CacheAddress()) + offsetof(env::detail::MemoryCache, physical))];
			sink[I::Local::Tee(actual)];

			/* continue the loop */
			sink[I::Branch::Direct(_loop)];
		}

		/* add the final unreachable (as the loop will never be left) */
		sink[I::Unreachable()];
	}

	/* add the write-lookup-wrapper function, which performs the lookup and writes the actual value to memory (will leave
	*	the cache at the last-most address - meaning another contiguous write will have a valid cache) */
	prototype = gen::Module->prototype(u8"mem_wrapper_write_type", { { u8"address", wasm::Type::i64 }, { u8"access", wasm::Type::i64 },
		{ u8"size", wasm::Type::i32 }, { u8"usage", wasm::Type::i32 }, { u8"cache", wasm::Type::i32 }, { u8"value", wasm::Type::i64 } }, {});
	wasm::Function writeLookupWrapper = gen::Module->function(u8"mem_wrapper_write", prototype, wasm::Export{});
	{
		wasm::Sink sink{ writeLookupWrapper };
		wasm::Variable cacheOffset = sink.local(wasm::Type::i32, u8"mem_lookup_cache");
		wasm::Variable offset = sink.local(wasm::Type::i32, u8"mem_lookup_offset");
		wasm::Variable size = sink.local(wasm::Type::i32, u8"mem_lookup_total");
		wasm::Variable actual = sink.local(wasm::Type::i32, u8"mem_lookup_physical");
		wasm::Variable index = sink.local(wasm::Type::i32, u8"mem_lookup_index");

		/* compute the base cache address */
		sink[I::Param::Get(4)];
		sink[I::U32::Const(sizeof(env::detail::MemoryCache))];
		sink[I::U32::Mul()];
		sink[I::Local::Set(cacheOffset)];

		/* perform the call for the base address (will automatically populate the cache properly) */
		sink[I::Param::Get(0)];
		sink[I::Param::Get(1)];
		sink[I::Param::Get(2)];
		sink[I::Param::Get(3)];
		sink[I::Param::Get(4)];
		sink[I::Call::Direct(pLookup)];

		/* compute the offset of the accessed address into the current cache entry */
		sink[I::Param::Get(1)];
		sink[I::Local::Get(cacheOffset)];
		sink[I::U64::Load(memory, uint32_t(env::detail::MemoryAccess::CacheAddress()) + offsetof(env::detail::MemoryCache, address))];
		sink[I::U64::Sub()];
		sink[I::U64::Shrink()];
		sink[I::Local::Tee(offset)];

		/* compute the physical address */
		sink[I::Local::Get(cacheOffset)];
		sink[I::U32::Load(memory, uint32_t(env::detail::MemoryAccess::CacheAddress()) + offsetof(env::detail::MemoryCache, physical))];
		sink[I::U32::Add()];
		sink[I::Local::Tee(actual)];

		/* check if the entire value can be read form the single cache (lower bound will match, assured by lookup) */
		sink[I::Local::Get(offset)];
		sink[I::Param::Get(2)];
		sink[I::U32::Add()];
		sink[I::Local::Get(cacheOffset)];
		sink[I::U32::Load(memory, uint32_t(env::detail::MemoryAccess::CacheAddress()) + offsetof(env::detail::MemoryCache, total))];
		sink[I::Local::Tee(size)];
		sink[I::U32::LessEqual()];

		/* the entire values lies in the single cache-entry (default case) */
		{
			wasm::IfThen _if{ sink, u8"", { wasm::Type::i32 }, { wasm::Type::i32 } };

			/* perform the sized write */
			sink[I::Param::Get(5)];
			wasm::Block _block8{ sink, u8"size_8", { wasm::Type::i32, wasm::Type::i64 }, { wasm::Type::i32, wasm::Type::i64 } };
			wasm::Block _block4{ sink, u8"size_4", { wasm::Type::i32, wasm::Type::i64 }, { wasm::Type::i32, wasm::Type::i64 } };
			wasm::Block _block2{ sink, u8"size_2", { wasm::Type::i32, wasm::Type::i64 }, { wasm::Type::i32, wasm::Type::i64 } };
			wasm::Block _block1{ sink, u8"size_1", { wasm::Type::i32, wasm::Type::i64 }, { wasm::Type::i32, wasm::Type::i64 } };
			sink[I::Param::Get(2)];
			sink[I::U32::TrailingNulls()];
			sink[I::Branch::Table({ _block1, _block2, _block4 }, _block8)];

			/* add the size-1 write */
			_block1.close();
			sink[I::U64::Store8(physical)];
			sink[I::Return()];

			/* add the size-2 write */
			_block2.close();
			sink[I::U64::Store16(physical)];
			sink[I::Return()];

			/* add the size-4 write */
			_block4.close();
			sink[I::U64::Store32(physical)];
			sink[I::Return()];

			/* add the size-8 write */
			_block8.close();
			sink[I::U64::Store(physical)];
			sink[I::Return()];
		}

		/* update the size counter to contain the number of available bytes */
		sink[I::Local::Get(size)];
		sink[I::Local::Get(offset)];
		sink[I::U32::Sub()];
		sink[I::Local::Set(size)];

		/* write the value byte-wise */
		{
			wasm::Loop _loop{ sink, u8"", { wasm::Type::i32 }, {} };

			/* fetch the next byte to be written and shift it out of the value */
			sink[I::Param::Get(5)];
			sink[I::Param::Get(5)];
			sink[I::U64::Const(8)];
			sink[I::U64::ShiftRight()];
			sink[I::Param::Set(5)];

			/* write the next byte to memory */
			sink[I::U64::Store8(physical, 0)];

			/* advance the index */
			sink[I::U32::Const(1)];
			sink[I::Local::Get(index)];
			sink[I::U32::Add()];
			sink[I::Local::Tee(index)];

			/* check if the end has been reached */
			sink[I::Param::Get(2)];
			sink[I::U32::GreaterEqual()];
			{
				wasm::IfThen _if{ sink };
				sink[I::Return()];
			}

			/* check if the next byte still lies in the current cache entry */
			sink[I::Local::Get(index)];
			sink[I::Local::Get(size)];
			sink[I::U32::Less()];
			{
				wasm::IfThen _if{ sink };

				/* advance the physical address and write it to the stack */
				sink[I::Local::Get(actual)];
				sink[I::U32::Const(1)];
				sink[I::U32::Add()];
				sink[I::Local::Tee(actual)];
				sink[I::Branch::Direct(_loop)];
			}

			/* perform the next cache lookup */
			sink[I::Param::Get(0)];
			sink[I::Param::Get(1)];
			sink[I::Local::Get(index)];
			sink[I::U32::Expand()];
			sink[I::U64::Add()];
			sink[I::Param::Get(2)];
			sink[I::Param::Get(3)];
			sink[I::Param::Get(4)];
			sink[I::Call::Direct(pLookup)];

			/* fetch the new remaining total-counter */
			sink[I::Local::Get(cacheOffset)];
			sink[I::U32::Load(memory, uint32_t(env::detail::MemoryAccess::CacheAddress()) + offsetof(env::detail::MemoryCache, total))];
			sink[I::Local::Set(size)];

			/* fetch the new actual address and leave it on the stack */
			sink[I::Local::Get(cacheOffset)];
			sink[I::U32::Load(memory, uint32_t(env::detail::MemoryAccess::CacheAddress()) + offsetof(env::detail::MemoryCache, physical))];
			sink[I::Local::Tee(actual)];

			/* continue the loop */
			sink[I::Branch::Direct(_loop)];
		}
	}

	/* add the lookup functions */
	wasm::Function coreReadLookup[4] = {};
	wasm::Function coreWriteLookup[4] = {};
	wasm::Function coreCodeLookup[4] = {};
	{
		wasm::Prototype readPrototype = gen::Module->prototype(u8"mem_lookup_read_type", { { u8"address", wasm::Type::i64 }, { u8"access", wasm::Type::i64 } }, { wasm::Type::i64 });
		wasm::Prototype writePrototype = gen::Module->prototype(u8"mem_lookup_write_type", { { u8"address", wasm::Type::i64 }, { u8"access", wasm::Type::i64 }, { u8"value", wasm::Type::i64 } }, {});

		/* iteratively create the lookups for all size/cache-indices combinations */
		size_t caches = env::detail::MemoryAccess::CacheCount();
		for (size_t i = 0; i < caches * 4; ++i) {
			size_t cache = (i / 4), size = MemoryBuilder::Sizes[i % 4];

			/* create the read-function and add the implementation */
			wasm::Function read = gen::Module->function(str::u8::Build(u8"mem_lookup_read_", size, u8'_', cache), readPrototype, wasm::Export{});
			state.reads.push_back(read);
			{
				wasm::Sink sink{ read };

				/* perform the actual parameterized lookup-call */
				sink[I::Param::Get(0)];
				sink[I::Param::Get(1)];
				sink[I::U32::Const(size)];
				sink[I::U32::Const(env::Usage::Read)];
				sink[I::U32::Const(cache)];
				sink[I::Call::Tail(readLookupWrapper)];
			}

			/* create the write-function and add the implementation */
			wasm::Function write = gen::Module->function(str::u8::Build(u8"mem_lookup_write_", size, u8'_', cache), writePrototype, wasm::Export{});
			state.writes.push_back(write);
			{
				wasm::Sink sink{ write };

				/* perform the actual parameterized lookup-call */
				sink[I::Param::Get(0)];
				sink[I::Param::Get(1)];
				sink[I::U32::Const(size)];
				sink[I::U32::Const(env::Usage::Write)];
				sink[I::U32::Const(cache)];
				sink[I::Param::Get(2)];
				sink[I::Call::Tail(writeLookupWrapper)];
			}
		}

		/* create the core lookup-functions */
		for (size_t i = 0; i < 4; ++i) {
			size_t size = MemoryBuilder::Sizes[i];

			/* create the read-function */
			coreReadLookup[i] = gen::Module->function(str::u8::Build(u8"mem_core_lookup_read_", size), readPrototype);
			{
				wasm::Sink sink{ coreReadLookup[i] };

				/* perform the actual parameterized lookup-call */
				sink[I::Param::Get(0)];
				sink[I::Param::Get(1)];
				sink[I::U32::Const(size)];
				sink[I::U32::Const(env::Usage::Read)];
				sink[I::U32::Const(env::detail::MemoryAccess::ReadCache())];
				sink[I::Call::Tail(readLookupWrapper)];
			}

			/* create the write-function */
			coreWriteLookup[i] = gen::Module->function(str::u8::Build(u8"mem_core_lookup_write_", size), writePrototype);
			{
				wasm::Sink sink{ coreWriteLookup[i] };

				/* perform the actual parameterized lookup-call */
				sink[I::Param::Get(0)];
				sink[I::Param::Get(1)];
				sink[I::U32::Const(size)];
				sink[I::U32::Const(env::Usage::Write)];
				sink[I::U32::Const(env::detail::MemoryAccess::WriteCache())];
				sink[I::Param::Get(2)];
				sink[I::Call::Tail(writeLookupWrapper)];
			}

			/* create the code-function */
			coreCodeLookup[i] = gen::Module->function(str::u8::Build(u8"mem_core_lookup_code_", size), readPrototype);
			{
				wasm::Sink sink{ coreCodeLookup[i] };

				/* perform the actual parameterized lookup-call */
				sink[I::Param::Get(0)];
				sink[I::Param::Get(1)];
				sink[I::U32::Const(size)];
				sink[I::U32::Const(env::Usage::Execute)];
				sink[I::U32::Const(env::detail::MemoryAccess::CodeCache())];
				sink[I::Call::Tail(readLookupWrapper)];
			}
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
		_writer.fMakeStopWrite(env::detail::MemoryAccess::WriteCache(), gen::MemoryType::u8To64, env::detail::MainAccessAddress, coreWriteLookup + 0);
		sink[I::Return()];

		_block2.close();
		_writer.fMakeStartWrite(env::detail::MemoryAccess::WriteCache(), gen::MemoryType::u16To64, env::detail::MainAccessAddress, coreWriteLookup + 1);
		sink[I::Param::Get(2)];
		_writer.fMakeStopWrite(env::detail::MemoryAccess::WriteCache(), gen::MemoryType::u16To64, env::detail::MainAccessAddress, coreWriteLookup + 1);
		sink[I::Return()];

		_block4.close();
		_writer.fMakeStartWrite(env::detail::MemoryAccess::WriteCache(), gen::MemoryType::u32To64, env::detail::MainAccessAddress, coreWriteLookup + 2);
		sink[I::Param::Get(2)];
		_writer.fMakeStopWrite(env::detail::MemoryAccess::WriteCache(), gen::MemoryType::u32To64, env::detail::MainAccessAddress, coreWriteLookup + 2);
		sink[I::Return()];

		_block8.close();
		_writer.fMakeStartWrite(env::detail::MemoryAccess::WriteCache(), gen::MemoryType::i64, env::detail::MainAccessAddress, coreWriteLookup + 3);
		sink[I::Param::Get(2)];
		_writer.fMakeStopWrite(env::detail::MemoryAccess::WriteCache(), gen::MemoryType::i64, env::detail::MainAccessAddress, coreWriteLookup + 3);

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

	/* setup the prototypes for the lookup functions */
	wasm::Prototype readPrototype = gen::Module->prototype(u8"mem_lookup_read_type", { { u8"address", wasm::Type::i64 }, { u8"access", wasm::Type::i64 } }, { wasm::Type::i64 });
	wasm::Prototype writePrototype = gen::Module->prototype(u8"mem_lookup_write_type", { { u8"address", wasm::Type::i64 }, { u8"access", wasm::Type::i64 }, { u8"value", wasm::Type::i64 } }, {});

	/* setup the actual function imports to the lookup functions */
	size_t caches = env::detail::MemoryAccess::CacheCount();
	for (size_t i = 0; i < caches * 4; ++i) {
		size_t cache = (i / 4), size = MemoryBuilder::Sizes[i % 4];
		state.reads.push_back(gen::Module->function(str::u8::Build(u8"mem_lookup_read_", size, u8'_', cache), readPrototype, wasm::Import{ u8"mem" }));
		state.writes.push_back(gen::Module->function(str::u8::Build(u8"mem_lookup_write_", size, u8'_', cache), writePrototype, wasm::Import{ u8"mem" }));
	}
}
