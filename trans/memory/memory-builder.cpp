#include "memory-builder.h"
#include "memory-writer.h"

namespace I = wasm::inst;

trans::detail::MemoryBuilder::MemoryBuilder(env::Process* process) : pProcess{ process } {}

void trans::detail::MemoryBuilder::fMakeLookup(const detail::MemoryState& state, const wasm::Function& function, uint32_t usage) const {
	wasm::Sink sink{ function };
	wasm::Variable address = sink.parameter(0), size = sink.parameter(1), cacheAddress = sink.parameter(2);

	/* allocate the local variables necessary to write the temporary result back */
	wasm::Variable outAddr = sink.local(wasm::Type::i64, u8"out_address");
	wasm::Variable outPhys = sink.local(wasm::Type::i32, u8"out_physical");
	wasm::Variable outSize = sink.local(wasm::Type::i32, u8"out_size");

	/* perform the call */
	sink[I::U64::Const(pProcess)];
	sink[I::Local::Get(address)];
	sink[I::Local::Get(size)];
	sink[I::U32::Const(usage)];
	sink[I::Call::Direct(pLookup)];

	/* fetch the result */
	sink[I::U64::Const(pProcess)];
	sink[I::Call::Direct(pGetAddress)];
	sink[I::Local::Set(outAddr)];
	sink[I::U64::Const(pProcess)];
	sink[I::Call::Direct(pGetPhysical)];
	sink[I::Local::Set(outPhys)];
	sink[I::U64::Const(pProcess)];
	sink[I::Call::Direct(pGetSize)];
	sink[I::Local::Set(outSize)];

	/* write the address and physical offset back */
	sink[I::Local::Get(cacheAddress)];
	sink[I::Local::Get(outAddr)];
	sink[I::U64::Store(state.management, offsetof(env::detail::MemoryCache, address))];
	sink[I::Local::Get(cacheAddress)];
	sink[I::Local::Get(outPhys)];
	sink[I::U32::Store(state.management, offsetof(env::detail::MemoryCache, physical))];

	/* write the different sizes back */
	for (uint32_t i = 1; i <= 8; i *= 2) {
		sink[I::Local::Get(cacheAddress)];
		sink[I::Local::Get(outSize)];
		sink[I::U32::Const(i)];
		sink[I::U32::Sub()];
		switch (i) {
		case 1:
			sink[I::U32::Store(state.management, offsetof(env::detail::MemoryCache, size1))];
			break;
		case 2:
			sink[I::U32::Store(state.management, offsetof(env::detail::MemoryCache, size2))];
			break;
		case 4:
			sink[I::U32::Store(state.management, offsetof(env::detail::MemoryCache, size4))];
			break;
		case 8:
			sink[I::U32::Store(state.management, offsetof(env::detail::MemoryCache, size8))];
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
void trans::detail::MemoryBuilder::fMakeAccess(wasm::Module& mod, const detail::MemoryState& state, wasm::Type type, std::u8string_view name, trans::MemoryType memoryType) const {
	wasm::Sink read{ mod.function(str::Build<std::u8string>(u8"mem_read_", name), { wasm::Type::i64 }, { type }, wasm::Export{}) };
	wasm::Sink write{ mod.function(str::Build<std::u8string>(u8"mem_write_", name), { wasm::Type::i64, type }, {}, wasm::Export{}) };
	wasm::Sink code{ mod.function(str::Build<std::u8string>(u8"mem_code_", name), { wasm::Type::i64 }, { type }, wasm::Export{}) };

	/* make the read, write, and code function */
	read[I::Local::Get(read.parameter(0))];
	detail::MemoryWriter{ pProcess, read, state }.fMakeRead(env::detail::MemoryAccess{ pProcess }.readCache(), memoryType);

	code[I::Local::Get(code.parameter(0))];
	detail::MemoryWriter{ pProcess, code, state }.fMakeCode(env::detail::MemoryAccess{ pProcess }.codeCache(), memoryType);

	write[I::Local::Get(write.parameter(0))];
	detail::MemoryWriter{ pProcess, write, state }.fMakeWrite(write.parameter(1), env::detail::MemoryAccess{ pProcess }.writeCache(), memoryType);
}

void trans::detail::MemoryBuilder::setupCoreImports(wasm::Module& mod, detail::MemoryState& state) {
	/* add the import to the lookup-function */
	wasm::Prototype prototype = mod.prototype(u8"mem_lookup_type",
		{ { u8"process", wasm::Type::i64 }, { u8"addr", wasm::Type::i64 }, { u8"size", wasm::Type::i32 }, { u8"usage", wasm::Type::i32 } },
		{}
	);
	pLookup = mod.function(u8"lookup", prototype, wasm::Import{ u8"mem" });

	prototype = mod.prototype(u8"mem_lookup_i64_type", { { u8"process", wasm::Type::i64 } }, { wasm::Type::i64 });
	pGetAddress = mod.function(u8"result_address", prototype, wasm::Import{ u8"mem" });

	prototype = mod.prototype(u8"mem_lookup_i32_type", { { u8"process", wasm::Type::i64 } }, { wasm::Type::i32 });
	pGetPhysical = mod.function(u8"result_physical", prototype, wasm::Import{ u8"mem" });
	pGetSize = mod.function(u8"result_size", prototype, wasm::Import{ u8"mem" });
}
void trans::detail::MemoryBuilder::setupCoreBody(wasm::Module& mod, detail::MemoryState& state) const {
	uint32_t caches = env::detail::MemoryAccess{ pProcess }.caches();
	uint32_t cacheAddress = env::detail::MemoryAccess{ pProcess }.cacheAddress();

	/* add the functions for the page-patching (receive the address as parameter and return the new absolute address) */
	wasm::Prototype prototype = mod.prototype(u8"mem_lookup_usage_type", { { u8"addr", wasm::Type::i64 }, { u8"size", wasm::Type::i32 }, { u8"cache_address", wasm::Type::i32 } }, { wasm::Type::i32 });
	state.read = mod.function(u8"mem_lookup_read", prototype, wasm::Export{});
	state.write = mod.function(u8"mem_lookup_write", prototype, wasm::Export{});
	state.code = mod.function(u8"mem_lookup_code", prototype, wasm::Export{});

	/* add the actual implementations */
	fMakeLookup(state, state.read, env::MemoryUsage::Read);
	fMakeLookup(state, state.write, env::MemoryUsage::Write);
	fMakeLookup(state, state.code, env::MemoryUsage::Execute);

	/* add the separate access functions */
	fMakeAccess(mod, state, wasm::Type::i32, u8"u8_i32", trans::MemoryType::u8To32);
	fMakeAccess(mod, state, wasm::Type::i32, u8"i8_i32", trans::MemoryType::i8To32);
	fMakeAccess(mod, state, wasm::Type::i32, u8"u16_i32", trans::MemoryType::u16To32);
	fMakeAccess(mod, state, wasm::Type::i32, u8"i16_i32", trans::MemoryType::i16To32);
	fMakeAccess(mod, state, wasm::Type::i32, u8"i32", trans::MemoryType::i32);
	fMakeAccess(mod, state, wasm::Type::i64, u8"i64", trans::MemoryType::i64);
	fMakeAccess(mod, state, wasm::Type::f32, u8"f32", trans::MemoryType::f32);
	fMakeAccess(mod, state, wasm::Type::f64, u8"f64", trans::MemoryType::f64);

	/* add the cache-flushing function */
	{
		wasm::Sink sink{ mod.function(u8"mem_flush_caches", {}, {}, wasm::Export{}) };

		/* destination-address */
		sink[I::U32::Const(cacheAddress)];

		/* value */
		sink[I::U32::Const(0)];

		/* size */
		sink[I::U32::Const(uint64_t(caches + env::detail::InternalCaches) * sizeof(env::detail::MemoryCache))];

		sink[I::Memory::Fill(state.management)];
	}

	/* add the memory-expansion function */
	{
		wasm::Prototype expandPhysicalType = mod.prototype(u8"mem_expand_physical_type", { { u8"pages", wasm::Type::i32 } }, { wasm::Type::i32 });
		wasm::Sink sink{ mod.function(u8"mem_expand_physical", expandPhysicalType, wasm::Export{}) };

		/* number of pages to grow by */
		sink[I::Local::Get(sink.parameter(0))];

		sink[I::Memory::Grow(state.physical)];

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
		sink[I::Local::Get(sink.parameter(0))];

		/* source-address */
		sink[I::Local::Get(sink.parameter(1))];

		/* size */
		sink[I::Local::Get(sink.parameter(2))];

		sink[I::Memory::Copy(state.physical)];
	}
}
void trans::detail::MemoryBuilder::setupBlockImports(wasm::Module& mod, detail::MemoryState& state) const {
	/* add the function-imports for the page-lookup */
	wasm::Prototype prototype = mod.prototype(u8"mem_lookup_usage_type", { { u8"addr", wasm::Type::i64 }, { u8"size", wasm::Type::i32 }, { u8"cache", wasm::Type::i32 } }, { wasm::Type::i32 });
	state.read = mod.function(u8"lookup_read", prototype, wasm::Import{ u8"mem" });
	state.write = mod.function(u8"lookup_write", prototype, wasm::Import{ u8"mem" });
	state.code = mod.function(u8"lookup_code", prototype, wasm::Import{ u8"mem" });
}
