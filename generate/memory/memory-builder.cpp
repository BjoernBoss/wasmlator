#include "../generate.h"
#include "../environment/memory/env-memory.h"
#include "../environment/process/process-access.h"

void gen::detail::MemoryBuilder::fMakeLookup(const wasm::Memory& memory, const wasm::Function& function, uint32_t usage) const {
	wasm::Sink sink{ function };
	wasm::Variable cacheOffset = sink.local(wasm::Type::i32, u8"mem_lookup_cache");

	/* perform the call (will automatically populate the cache properly) */
	sink[I::Param::Get(0)];
	sink[I::Param::Get(1)];
	sink[I::Param::Get(2)];
	sink[I::U32::Const(usage)];
	sink[I::Param::Get(3)];
	sink[I::Call::Direct(pLookup)];

	/* compute the offset into the current cache-slot */
	sink[I::Param::Get(1)];
	sink[I::Param::Get(3)];
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
	env::detail::ProcessAccess::AddCoreBinding(u8"mem", u8"mem_lookup_read");
	env::detail::ProcessAccess::AddCoreBinding(u8"mem", u8"mem_lookup_write");
	env::detail::ProcessAccess::AddCoreBinding(u8"mem", u8"mem_lookup_code");
}
void gen::detail::MemoryBuilder::setupCoreBody(const wasm::Memory& memory, const wasm::Memory& physical) const {
	detail::MemoryState state{ {}, {}, {}, memory, physical };

	/* add the functions for the page-patching (receive the address as parameter and return the new absolute address) */
	wasm::Prototype prototype = gen::Module->prototype(u8"mem_lookup_usage_type", { { u8"address", wasm::Type::i64 }, { u8"access", wasm::Type::i64 }, { u8"size", wasm::Type::i32 }, { u8"cache", wasm::Type::i32 } }, { wasm::Type::i32 });
	state.read = gen::Module->function(u8"mem_lookup_read", prototype, wasm::Export{});
	state.write = gen::Module->function(u8"mem_lookup_write", prototype, wasm::Export{});
	state.code = gen::Module->function(u8"mem_lookup_code", prototype, wasm::Export{});

	/* add the actual implementations */
	fMakeLookup(memory, state.read, env::Usage::Read);
	fMakeLookup(memory, state.write, env::Usage::Write);
	fMakeLookup(memory, state.code, env::Usage::Execute);

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
		_writer.fMakeRead(env::detail::MemoryAccess::ReadCache(), gen::MemoryType::u8To64, env::detail::MainAccessAddress);
		sink[I::Return()];

		_block2.close();
		_writer.fMakeRead(env::detail::MemoryAccess::ReadCache(), gen::MemoryType::u16To64, env::detail::MainAccessAddress);
		sink[I::Return()];

		_block4.close();
		_writer.fMakeRead(env::detail::MemoryAccess::ReadCache(), gen::MemoryType::u32To64, env::detail::MainAccessAddress);
		sink[I::Return()];

		_block8.close();
		_writer.fMakeRead(env::detail::MemoryAccess::ReadCache(), gen::MemoryType::i64, env::detail::MainAccessAddress);

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
		sink[I::Param::Get(2)];
		wasm::Block _block8{ sink, u8"size_8", { wasm::Type::i64, wasm::Type::i64 }, { wasm::Type::i64, wasm::Type::i64 } };
		wasm::Block _block4{ sink, u8"size_4", { wasm::Type::i64, wasm::Type::i64 }, { wasm::Type::i64, wasm::Type::i64 } };
		wasm::Block _block2{ sink, u8"size_2", { wasm::Type::i64, wasm::Type::i64 }, { wasm::Type::i64, wasm::Type::i64 } };
		wasm::Block _block1{ sink, u8"size_1", { wasm::Type::i64, wasm::Type::i64 }, { wasm::Type::i64, wasm::Type::i64 } };
		sink[I::Param::Get(1)];
		sink[I::U32::TrailingNulls()];
		sink[I::Branch::Table({ _block1, _block2, _block4 }, _block8)];
		_block1.close();
		_writer.fMakeWrite(env::detail::MemoryAccess::WriteCache(), gen::MemoryType::u8To64, env::detail::MainAccessAddress);
		sink[I::Return()];

		_block2.close();
		_writer.fMakeWrite(env::detail::MemoryAccess::WriteCache(), gen::MemoryType::u16To64, env::detail::MainAccessAddress);
		sink[I::Return()];

		_block4.close();
		_writer.fMakeWrite(env::detail::MemoryAccess::WriteCache(), gen::MemoryType::u32To64, env::detail::MainAccessAddress);
		sink[I::Return()];

		_block8.close();
		_writer.fMakeWrite(env::detail::MemoryAccess::WriteCache(), gen::MemoryType::i64, env::detail::MainAccessAddress);

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
		_writer.fMakeCode(env::detail::MemoryAccess::CodeCache(), gen::MemoryType::u8To64, env::detail::MainAccessAddress);
		sink[I::Return()];

		_block2.close();
		_writer.fMakeCode(env::detail::MemoryAccess::CodeCache(), gen::MemoryType::u16To64, env::detail::MainAccessAddress);
		sink[I::Return()];

		_block4.close();
		_writer.fMakeCode(env::detail::MemoryAccess::CodeCache(), gen::MemoryType::u32To64, env::detail::MainAccessAddress);
		sink[I::Return()];

		_block8.close();
		_writer.fMakeCode(env::detail::MemoryAccess::CodeCache(), gen::MemoryType::i64, env::detail::MainAccessAddress);

		/* clear the sink reference */
		gen::Instance()->setSink(0);
	}
}
void gen::detail::MemoryBuilder::setupBlockImports(const wasm::Memory& memory, const wasm::Memory& physical, detail::MemoryState& state) const {
	state.memory = memory;
	state.physical = physical;

	/* add the function-imports for the page-lookup */
	wasm::Prototype prototype = gen::Module->prototype(u8"mem_lookup_usage_type", { { u8"address", wasm::Type::i64 }, { u8"access", wasm::Type::i64 }, { u8"size", wasm::Type::i32 }, { u8"cache", wasm::Type::i32 } }, { wasm::Type::i32 });
	state.read = gen::Module->function(u8"mem_lookup_read", prototype, wasm::Import{ u8"mem" });
	state.write = gen::Module->function(u8"mem_lookup_write", prototype, wasm::Import{ u8"mem" });
	state.code = gen::Module->function(u8"mem_lookup_code", prototype, wasm::Import{ u8"mem" });
}
