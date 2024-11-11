#include "memory-builder.h"
#include "memory-writer.h"

namespace I = wasm::inst;

void gen::detail::MemoryBuilder::fMakeLookup(const wasm::Memory& management, const wasm::Function& function, uint32_t usage) const {
	wasm::Sink sink{ function };
	wasm::Variable address = sink.param(0), size = sink.param(1), cacheAddress = sink.param(2);

	/* allocate the local variables necessary to write the temporary result back */
	wasm::Variable outAddr = sink.local(wasm::Type::i64, u8"out_address");
	wasm::Variable outPhys = sink.local(wasm::Type::i32, u8"out_physical");
	wasm::Variable outSize = sink.local(wasm::Type::i32, u8"out_size");

	/* perform the call */
	sink[I::Local::Get(address)];
	sink[I::Local::Get(size)];
	sink[I::U32::Const(usage)];
	sink[I::Call::Direct(pLookup)];

	/* fetch the result */
	sink[I::Call::Direct(pGetAddress)];
	sink[I::Local::Set(outAddr)];
	sink[I::Call::Direct(pGetPhysical)];
	sink[I::Local::Set(outPhys)];
	sink[I::Call::Direct(pGetSize)];
	sink[I::Local::Set(outSize)];

	/* write the address and physical offset back */
	sink[I::Local::Get(cacheAddress)];
	sink[I::Local::Get(outAddr)];
	sink[I::U64::Store(management, offsetof(env::detail::MemoryCache, address))];
	sink[I::Local::Get(cacheAddress)];
	sink[I::Local::Get(outPhys)];
	sink[I::U32::Store(management, offsetof(env::detail::MemoryCache, physical))];

	/* write the different sizes back */
	for (uint32_t i = 1; i <= 8; i *= 2) {
		sink[I::Local::Get(cacheAddress)];
		sink[I::Local::Get(outSize)];
		sink[I::U32::Const(i)];
		sink[I::U32::Sub()];
		switch (i) {
		case 1:
			sink[I::U32::Store(management, offsetof(env::detail::MemoryCache, size1))];
			break;
		case 2:
			sink[I::U32::Store(management, offsetof(env::detail::MemoryCache, size2))];
			break;
		case 4:
			sink[I::U32::Store(management, offsetof(env::detail::MemoryCache, size4))];
			break;
		case 8:
			sink[I::U32::Store(management, offsetof(env::detail::MemoryCache, size8))];
			break;
		}
	}

	/* compute the final actual offset */
	sink[I::Local::Get(address)];
	sink[I::Local::Get(outAddr)];
	sink[I::U64::Sub()];
	sink[I::U64::Shrink()];
	sink[I::Local::Get(outPhys)];
	sink[I::U32::Add()];
}

void gen::detail::MemoryBuilder::setupGlueMappings(detail::GlueState& glue) {
	glue.define(u8"mem_flush_caches", {}, {});
	glue.define(u8"mem_expand_physical", { { u8"pages", wasm::Type::i32 } }, { wasm::Type::i32 });
	glue.define(u8"mem_move_physical", { { u8"dest", wasm::Type::i32 }, { u8"source", wasm::Type::i32 }, { u8"size", wasm::Type::i32 } }, {});
	glue.define(u8"mem_read", { { u8"address", wasm::Type::i64 }, { u8"size", wasm::Type::i32 } }, { wasm::Type::i64 });
	glue.define(u8"mem_write", { { u8"address", wasm::Type::i64 }, { u8"size", wasm::Type::i32 }, { u8"value", wasm::Type::i64 } }, {});
	glue.define(u8"mem_code", { { u8"address", wasm::Type::i64 }, { u8"size", wasm::Type::i32 } }, { wasm::Type::i64 });
}
void gen::detail::MemoryBuilder::setupCoreImports(wasm::Module& mod) {
	/* add the import to the lookup-function */
	wasm::Prototype prototype = mod.prototype(u8"main_lookup_type", { { u8"address", wasm::Type::i64 }, { u8"size", wasm::Type::i32 }, { u8"usage", wasm::Type::i32 } }, {});
	pLookup = mod.function(u8"main_lookup", prototype, wasm::Import{ u8"main" });

	prototype = mod.prototype(u8"main_lookup_i64_type", {}, { wasm::Type::i64 });
	pGetAddress = mod.function(u8"main_result_address", prototype, wasm::Import{ u8"main" });

	prototype = mod.prototype(u8"main_lookup_i32_type", {}, { wasm::Type::i32 });
	pGetPhysical = mod.function(u8"main_result_physical", prototype, wasm::Import{ u8"main" });
	pGetSize = mod.function(u8"main_result_size", prototype, wasm::Import{ u8"main" });
}
void gen::detail::MemoryBuilder::setupCoreBody(wasm::Module& mod, const wasm::Memory& management, const wasm::Memory& physical) const {
	uint32_t caches = env::detail::MemoryAccess{}.caches();
	uint32_t cacheAddress = env::detail::MemoryAccess{}.cacheAddress();
	detail::MemoryState state{ {}, {}, {}, management, physical };

	/* add the functions for the page-patching (receive the address as parameter and return the new absolute address) */
	wasm::Prototype prototype = mod.prototype(u8"mem_lookup_usage_type", { { u8"address", wasm::Type::i64 }, { u8"size", wasm::Type::i32 }, { u8"cache", wasm::Type::i32 } }, { wasm::Type::i32 });
	state.read = mod.function(u8"mem_lookup_read", prototype, wasm::Export{});
	state.write = mod.function(u8"mem_lookup_write", prototype, wasm::Export{});
	state.code = mod.function(u8"mem_lookup_code", prototype, wasm::Export{});

	/* add the actual implementations */
	fMakeLookup(management, state.read, env::MemoryUsage::Read);
	fMakeLookup(management, state.write, env::MemoryUsage::Write);
	fMakeLookup(management, state.code, env::MemoryUsage::Execute);

	/* add the cache-flushing function */
	{
		wasm::Sink sink{ mod.function(u8"mem_flush_caches", {}, {}, wasm::Export{}) };

		/* destination-address */
		sink[I::U32::Const(cacheAddress)];

		/* value */
		sink[I::U32::Const(0)];

		/* size */
		sink[I::U32::Const(uint64_t(caches + env::detail::InternalCaches) * sizeof(env::detail::MemoryCache))];

		sink[I::Memory::Fill(management)];
	}

	/* add the memory-expansion function */
	{
		wasm::Prototype expandPhysicalType = mod.prototype(u8"mem_expand_physical_type", { { u8"pages", wasm::Type::i32 } }, { wasm::Type::i32 });
		wasm::Sink sink{ mod.function(u8"mem_expand_physical", expandPhysicalType, wasm::Export{}) };

		/* number of pages to grow by */
		sink[I::Param::Get(0)];

		sink[I::Memory::Grow(physical)];

		/* convert result to 1 or 0 */
		sink[I::I32::Const(0)];
		sink[I::I32::Less()];
		{
			wasm::IfThen _if{ sink, {}, {}, {wasm::Type::i32 } };
			sink[I::U32::Const(0)];
			_if.otherwise();
			sink[I::U32::Const(1)];
		}
	}

	/* add the memory-move function */
	{
		wasm::Prototype movePhysicalType = mod.prototype(u8"mem_move_physical_type", { { u8"dest", wasm::Type::i32 }, { u8"source", wasm::Type::i32 }, { u8"size", wasm::Type::i32 } }, {});
		wasm::Sink sink{ mod.function(u8"mem_move_physical", movePhysicalType, wasm::Export{}) };

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
		prototype = mod.prototype(u8"mem_read", { { u8"address", wasm::Type::i64 }, { u8"size", wasm::Type::i32 } }, { wasm::Type::i64 });
		wasm::Sink sink{ mod.function(u8"mem_read", prototype, wasm::Export{}) };
		detail::MemoryWriter _writer{ state, sink };

		sink[I::Param::Get(0)];
		wasm::Block _block1{ sink, u8"size_1", { wasm::Type::i64 }, { wasm::Type::i64 } };
		wasm::Block _block2{ sink, u8"size_2", { wasm::Type::i64 }, { wasm::Type::i64 } };
		wasm::Block _block4{ sink, u8"size_4", { wasm::Type::i64 }, { wasm::Type::i64 } };
		wasm::Block _block8{ sink, u8"size_8", { wasm::Type::i64 }, { wasm::Type::i64 } };
		sink[I::Param::Get(1)];
		sink[I::U32::TrailingNulls()];
		sink[I::Branch::Table({ _block1, _block2, _block4 }, _block8)];
		_block1.close();
		_writer.fMakeRead(env::detail::MemoryAccess{}.readCache(), gen::MemoryType::u8To64);
		sink[I::Return()];

		_block2.close();
		_writer.fMakeRead(env::detail::MemoryAccess{}.readCache(), gen::MemoryType::u16To64);
		sink[I::Return()];

		_block4.close();
		_writer.fMakeRead(env::detail::MemoryAccess{}.readCache(), gen::MemoryType::u32To64);
		sink[I::Return()];

		_block8.close();
		_writer.fMakeRead(env::detail::MemoryAccess{}.readCache(), gen::MemoryType::i64);
	}

	/* add the write-access function */
	{
		prototype = mod.prototype(u8"mem_write", { { u8"address", wasm::Type::i64 }, { u8"size", wasm::Type::i32 }, { u8"value", wasm::Type::i64 } }, {});
		wasm::Sink sink{ mod.function(u8"mem_write", prototype, wasm::Export{}) };
		detail::MemoryWriter _writer{ state, sink };

		sink[I::Param::Get(0)];
		wasm::Block _block1{ sink, u8"size_1", { wasm::Type::i64 }, { wasm::Type::i64 } };
		wasm::Block _block2{ sink, u8"size_2", { wasm::Type::i64 }, { wasm::Type::i64 } };
		wasm::Block _block4{ sink, u8"size_4", { wasm::Type::i64 }, { wasm::Type::i64 } };
		wasm::Block _block8{ sink, u8"size_8", { wasm::Type::i64 }, { wasm::Type::i64 } };
		sink[I::Param::Get(1)];
		sink[I::U32::TrailingNulls()];
		sink[I::Branch::Table({ _block1, _block2, _block4 }, _block8)];
		_block1.close();
		_writer.fMakeWrite(sink.param(2), env::detail::MemoryAccess{}.writeCache(), gen::MemoryType::u8To64);
		sink[I::Return()];

		_block2.close();
		_writer.fMakeWrite(sink.param(2), env::detail::MemoryAccess{}.writeCache(), gen::MemoryType::u16To64);
		sink[I::Return()];

		_block4.close();
		_writer.fMakeWrite(sink.param(2), env::detail::MemoryAccess{}.writeCache(), gen::MemoryType::u32To64);
		sink[I::Return()];

		_block8.close();
		_writer.fMakeWrite(sink.param(2), env::detail::MemoryAccess{}.writeCache(), gen::MemoryType::i64);
	}

	/* add the code-access function */
	{
		prototype = mod.prototype(u8"mem_code", { { u8"address", wasm::Type::i64 }, { u8"size", wasm::Type::i32 } }, { wasm::Type::i64 });
		wasm::Sink sink{ mod.function(u8"mem_code", prototype, wasm::Export{}) };
		detail::MemoryWriter _writer{ state, sink };

		sink[I::Param::Get(0)];
		wasm::Block _block1{ sink, u8"size_1", { wasm::Type::i64 }, { wasm::Type::i64 } };
		wasm::Block _block2{ sink, u8"size_2", { wasm::Type::i64 }, { wasm::Type::i64 } };
		wasm::Block _block4{ sink, u8"size_4", { wasm::Type::i64 }, { wasm::Type::i64 } };
		wasm::Block _block8{ sink, u8"size_8", { wasm::Type::i64 }, { wasm::Type::i64 } };
		sink[I::Param::Get(1)];
		sink[I::U32::TrailingNulls()];
		sink[I::Branch::Table({ _block1, _block2, _block4 }, _block8)];
		_block1.close();
		_writer.fMakeCode(env::detail::MemoryAccess{}.codeCache(), gen::MemoryType::u8To64);
		sink[I::Return()];

		_block2.close();
		_writer.fMakeCode(env::detail::MemoryAccess{}.codeCache(), gen::MemoryType::u16To64);
		sink[I::Return()];

		_block4.close();
		_writer.fMakeCode(env::detail::MemoryAccess{}.codeCache(), gen::MemoryType::u32To64);
		sink[I::Return()];

		_block8.close();
		_writer.fMakeCode(env::detail::MemoryAccess{}.codeCache(), gen::MemoryType::i64);
	}
}
void gen::detail::MemoryBuilder::setupBlockImports(wasm::Module& mod, const wasm::Memory& management, const wasm::Memory& physical, detail::MemoryState& state) const {
	state.management = management;
	state.physical = physical;

	/* add the function-imports for the page-lookup */
	wasm::Prototype prototype = mod.prototype(u8"mem_lookup_usage_type", { { u8"address", wasm::Type::i64 }, { u8"size", wasm::Type::i32 }, { u8"cache", wasm::Type::i32 } }, { wasm::Type::i32 });
	state.read = mod.function(u8"mem_lookup_read", prototype, wasm::Import{ u8"core" });
	state.write = mod.function(u8"mem_lookup_write", prototype, wasm::Import{ u8"core" });
	state.code = mod.function(u8"mem_lookup_code", prototype, wasm::Import{ u8"core" });
}
