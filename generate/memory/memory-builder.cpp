#include "../generate.h"
#include "../../environment/environment.h"

wasm::Function gen::detail::MemoryBuilder::fMakeReadWrapper(const wasm::Memory& memory, const wasm::Memory& physical) const {
	wasm::Prototype prototype = gen::Module->prototype(u8"mem_wrapper_read_type", { { u8"address", wasm::Type::i64 }, { u8"access", wasm::Type::i64 },
		{ u8"size", wasm::Type::i32 }, { u8"usage", wasm::Type::i32 }, { u8"cache", wasm::Type::i32 } }, { wasm::Type::i64 });

	/* add the read-lookup-wrapper function, which performs the lookup and returns the actual value (will leave
	*	the cache at the last-most address - meaning another contiguous read will have a valid cache) */
	wasm::Function fn = gen::Module->function(u8"mem_wrapper_read", prototype, wasm::Export{});
	wasm::Sink sink{ fn };
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

	/* perform the lookup call for the base address (will automatically populate the cache properly) */
	sink[I::Param::Get(0)];
	sink[I::Param::Get(1)];
	sink[I::Param::Get(2)];
	sink[I::Param::Get(3)];
	sink[I::Param::Get(4)];
	sink[I::Call::Direct(pCheckLookup)];

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

	/* check if the entire value can be read from the single cache (lower bound will match, assured by lookup - size1 is equivalent to total size) */
	sink[I::Local::Get(offset)];
	sink[I::Param::Get(2)];
	sink[I::U32::Add()];
	sink[I::Local::Get(cacheOffset)];
	sink[I::U32::Load(memory, uint32_t(env::detail::MemoryAccess::CacheAddress()) + offsetof(env::detail::MemoryCache, size1))];
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

		/* perform the next fast cache lookup */
		sink[I::Param::Get(1)];
		sink[I::Local::Get(index)];
		sink[I::U32::Expand()];
		sink[I::U64::Add()];
		sink[I::Param::Get(3)];
		sink[I::Param::Get(4)];
		sink[I::Call::Direct(pFastLookup)];

		/* fetch the new remaining total-counter (size1 is equivalent to total size) */
		sink[I::Local::Get(cacheOffset)];
		sink[I::U32::Load(memory, uint32_t(env::detail::MemoryAccess::CacheAddress()) + offsetof(env::detail::MemoryCache, size1))];
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
	return fn;
}
wasm::Function gen::detail::MemoryBuilder::fMakeWriteWrapper(const wasm::Memory& memory, const wasm::Memory& physical) const {
	std::vector<wasm::Param> params{ { u8"address", wasm::Type::i64 }, { u8"access", wasm::Type::i64 }, { u8"size", wasm::Type::i32 }, { u8"usage", wasm::Type::i32 }, { u8"cache", wasm::Type::i32 }, { u8"value", wasm::Type::i64 } };
	if (env::Instance()->detectWriteExecute())
		params.push_back({ u8"inst_size", wasm::Type::i32 });
	wasm::Prototype prototype = gen::Module->prototype(u8"mem_wrapper_write_type", params, {});

	/* add the write-lookup-wrapper function, which performs the lookup and writes the actual value to memory (will leave
	*	the cache at the last-most address - meaning another contiguous write will have a valid cache) */
	wasm::Function fn = gen::Module->function(u8"mem_wrapper_write", prototype, wasm::Export{});
	wasm::Sink sink{ fn };
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

	/* perform the lookup call for the base address (will automatically populate the cache properly) */
	sink[I::Param::Get(0)];
	sink[I::Param::Get(1)];
	sink[I::Param::Get(2)];
	sink[I::Param::Get(3)];
	sink[I::Param::Get(4)];
	sink[I::Call::Direct(pCheckLookup)];

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

	/* check if the entire value can be read form the single cache (lower bound will match, assured by lookup - size1 is equivalent to total size) */
	sink[I::Local::Get(offset)];
	sink[I::Param::Get(2)];
	sink[I::U32::Add()];
	sink[I::Local::Get(cacheOffset)];
	sink[I::U32::Load(memory, uint32_t(env::detail::MemoryAccess::CacheAddress()) + offsetof(env::detail::MemoryCache, size1))];
	sink[I::Local::Tee(size)];
	sink[I::U32::LessEqual()];

	/* the entire values lies in the single cache-entry (default case) */
	{
		wasm::IfThen _if{ sink, u8"", { wasm::Type::i32 }, { wasm::Type::i32 } };

		/* perform the sized write */
		sink[I::Param::Get(5)];
		wasm::Block _blockOut{ sink, u8"out", { wasm::Type::i32, wasm::Type::i64 }, {} };
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
		sink[I::Branch::Direct(_blockOut)];

		/* add the size-2 write */
		_block2.close();
		sink[I::U64::Store16(physical)];
		sink[I::Branch::Direct(_blockOut)];

		/* add the size-4 write */
		_block4.close();
		sink[I::U64::Store32(physical)];
		sink[I::Branch::Direct(_blockOut)];

		/* add the size-8 write */
		_block8.close();
		sink[I::U64::Store(physical)];

		/* add the final check for a write-to-execute and otherwise add the return */
		_blockOut.close();
		if (env::Instance()->detectWriteExecute()) {
			/* compute the address of the next address */
			sink[I::Param::Get(0)];
			sink[I::Param::Get(6)];
			sink[I::U32::Expand()];
			sink[I::U64::Add()];

			/* perform the check if this write wrote to an executable page */
			sink[I::Call::Tail(pCheckXWrite)];
		}
		else
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

			if (env::Instance()->detectWriteExecute()) {
				/* compute the address of the next address */
				sink[I::Param::Get(0)];
				sink[I::Param::Get(6)];
				sink[I::U32::Expand()];
				sink[I::U64::Add()];

				/* perform the check if this write wrote to an executable page */
				sink[I::Call::Tail(pCheckXWrite)];
			}
			else
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

		/* perform the next fast cache lookup */
		sink[I::Param::Get(1)];
		sink[I::Local::Get(index)];
		sink[I::U32::Expand()];
		sink[I::U64::Add()];
		sink[I::Param::Get(3)];
		sink[I::Param::Get(4)];
		sink[I::Call::Direct(pFastLookup)];

		/* fetch the new remaining total-counter (size1 is equivalent to total size) */
		sink[I::Local::Get(cacheOffset)];
		sink[I::U32::Load(memory, uint32_t(env::detail::MemoryAccess::CacheAddress()) + offsetof(env::detail::MemoryCache, size1))];
		sink[I::Local::Set(size)];

		/* fetch the new actual address and leave it on the stack */
		sink[I::Local::Get(cacheOffset)];
		sink[I::U32::Load(memory, uint32_t(env::detail::MemoryAccess::CacheAddress()) + offsetof(env::detail::MemoryCache, physical))];
		sink[I::Local::Tee(actual)];

		/* continue the loop */
		sink[I::Branch::Direct(_loop)];
	}
	return fn;
}
wasm::Function gen::detail::MemoryBuilder::fMakeReadLookup(uint32_t size, bool read, gen::MemoryType type, const wasm::Prototype& prototype, const wasm::Function& wrapper) const {
	/* create the read-function and add the implementation */
	wasm::Function fn = gen::Module->function(str::u8::Build(u8"mem_lookup_", (read ? u8"read_" : u8"code_"), size_t(type)), prototype, wasm::Export{});
	wasm::Sink sink{ fn };

	/* perform the actual parameterized lookup-call */
	sink[I::Param::Get(0)];
	sink[I::Param::Get(1)];
	sink[I::U32::Const(size)];
	sink[I::U32::Const(read ? env::Usage::Read : env::Usage::Execute)];
	sink[I::Param::Get(2)];
	sink[I::Call::Direct(wrapper)];

	/* transform the value to match the output type */
	switch (type) {
	case gen::MemoryType::u8To64:
	case gen::MemoryType::u16To64:
	case gen::MemoryType::u32To64:
	case gen::MemoryType::i64:
		break;
	case gen::MemoryType::i8To64:
		sink[I::U64::Const(54)];
		sink[I::U64::ShiftLeft()];
		sink[I::U64::Const(54)];
		sink[I::I64::ShiftRight()];
		break;
	case gen::MemoryType::i16To64:
		sink[I::U64::Const(48)];
		sink[I::U64::ShiftLeft()];
		sink[I::U64::Const(48)];
		sink[I::I64::ShiftRight()];
		break;
	case gen::MemoryType::i32To64:
		sink[I::U64::Shrink()];
		sink[I::I32::Expand()];
		break;
	case gen::MemoryType::f32:
		sink[I::U64::Shrink()];
		sink[I::U32::AsFloat()];
		break;
	case gen::MemoryType::f64:
		sink[I::U64::AsFloat()];
		break;
	case gen::MemoryType::u8To32:
	case gen::MemoryType::u16To32:
	case gen::MemoryType::i32:
		sink[I::U64::Shrink()];
		break;
	case gen::MemoryType::i8To32:
		sink[I::U64::Shrink()];
		sink[I::U32::Const(24)];
		sink[I::U32::ShiftLeft()];
		sink[I::U32::Const(24)];
		sink[I::I32::ShiftRight()];
		break;
	case gen::MemoryType::i16To32:
		sink[I::U64::Shrink()];
		sink[I::U32::Const(16)];
		sink[I::U32::ShiftLeft()];
		sink[I::U32::Const(16)];
		sink[I::I32::ShiftRight()];
		break;
	default:
		break;
	}
	return fn;
}
wasm::Function gen::detail::MemoryBuilder::fMakeWriteLookup(uint32_t size, gen::MemoryType type, const wasm::Prototype& prototype, const wasm::Function& wrapper) const {
	wasm::Function fn = gen::Module->function(str::u8::Build(u8"mem_lookup_write_", size_t(type)), prototype, wasm::Export{});
	wasm::Sink sink{ fn };

	/* setup the parameter for the call */
	sink[I::Param::Get(0)];
	sink[I::Param::Get(1)];
	sink[I::U32::Const(size)];
	sink[I::U32::Const(env::Usage::Write)];
	sink[I::Param::Get(2)];

	/* transform the value to the writable i64 */
	sink[I::Param::Get(3)];
	switch (type) {
	case gen::MemoryType::i64:
	case gen::MemoryType::u8To64:
	case gen::MemoryType::i8To64:
	case gen::MemoryType::u16To64:
	case gen::MemoryType::i16To64:
	case gen::MemoryType::u32To64:
	case gen::MemoryType::i32To64:
		break;
	case gen::MemoryType::f32:
		sink[I::F32::AsInt()];
		sink[I::U32::Expand()];
		break;
	case gen::MemoryType::f64:
		sink[I::F64::AsInt()];
		break;
	case gen::MemoryType::i32:
	case gen::MemoryType::u8To32:
	case gen::MemoryType::i8To32:
	case gen::MemoryType::u16To32:
	case gen::MemoryType::i16To32:
		sink[I::U32::Expand()];
		break;
	default:
		break;
	}

	/* check if the inst_size argument should be passed along */
	if (env::Instance()->detectWriteExecute())
		sink[I::Param::Get(4)];

	/* perform the actual call */
	sink[I::Call::Tail(wrapper)];
	return fn;
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
	/* add the import to the lookup-functions */
	wasm::Prototype prototype = gen::Module->prototype(u8"main_check_lookup_type", { { u8"address", wasm::Type::i64 }, { u8"access", wasm::Type::i64 }, { u8"size", wasm::Type::i32 }, { u8"usage", wasm::Type::i32 }, { u8"cache", wasm::Type::i32 } }, {});
	pCheckLookup = gen::Module->function(u8"main_check_lookup", prototype, wasm::Import{ u8"main" });
	prototype = gen::Module->prototype(u8"main_fast_lookup_type", { { u8"access", wasm::Type::i64 }, { u8"usage", wasm::Type::i32 }, { u8"cache", wasm::Type::i32 } }, {});
	pFastLookup = gen::Module->function(u8"main_fast_lookup", prototype, wasm::Import{ u8"main" });

	/* add the import to the check-x-write function */
	prototype = gen::Module->prototype(u8"main_check_invalidated_type", { { u8"address", wasm::Type::i64 } }, {});
	pCheckXWrite = gen::Module->function(u8"main_check_invalidated", prototype, wasm::Import{ u8"main" });

	/* define the bindings passed to the blocks */
	for (size_t i = 0; i < size_t(gen::MemoryType::_end); ++i) {
		env::detail::ProcessAccess::AddCoreBinding(u8"mem", str::u8::Build(u8"mem_lookup_read_", i));
		env::detail::ProcessAccess::AddCoreBinding(u8"mem", str::u8::Build(u8"mem_lookup_write_", i));
	}
}
void gen::detail::MemoryBuilder::setupCoreBody(const wasm::Memory& memory, const wasm::Memory& physical) const {
	detail::MemoryState state{ memory, physical };

	/* setup the two lookup-wrapper functions */
	wasm::Function readWrapper = fMakeReadWrapper(memory, physical);
	wasm::Function writeWrapper = fMakeWriteWrapper(memory, physical);

	/* add the lookup functions */
	wasm::Function codeLookup[4] = {};
	{
		/* iteratively create the lookups for all size/cache-indices combinations */
		for (size_t i = 0; i < size_t(gen::MemoryType::_end); ++i) {
			wasm::Type type = wasm::Type::i32;
			uint32_t size = 0, codeIndex = 4;

			/* resolve the wasm-type and size and code-index */
			switch (static_cast<gen::MemoryType>(i)) {
			case gen::MemoryType::u8To64:
				codeIndex = 0;
				[[fallthrough]];
			case gen::MemoryType::i8To64:
				size = 1;
				type = wasm::Type::i64;
				break;
			case gen::MemoryType::u16To64:
				codeIndex = 1;
				[[fallthrough]];
			case gen::MemoryType::i16To64:
				size = 2;
				type = wasm::Type::i64;
				break;
			case gen::MemoryType::u32To64:
				codeIndex = 2;
				[[fallthrough]];
			case gen::MemoryType::i32To64:
				size = 4;
				type = wasm::Type::i64;
				break;
			case gen::MemoryType::i64:
				codeIndex = 3;
				size = 8;
				type = wasm::Type::i64;
				break;
			case gen::MemoryType::f32:
				size = 4;
				type = wasm::Type::f32;
				break;
			case gen::MemoryType::f64:
				size = 8;
				type = wasm::Type::f64;
				break;
			case gen::MemoryType::u8To32:
			case gen::MemoryType::i8To32:
				size = 1;
				type = wasm::Type::i32;
				break;
			case gen::MemoryType::u16To32:
			case gen::MemoryType::i16To32:
				size = 2;
				type = wasm::Type::i32;
				break;
			case gen::MemoryType::i32:
				size = 4;
				type = wasm::Type::i32;
				break;
			default:
				break;
			}

			/* setup the read-prototype */
			wasm::Prototype readPrototype = gen::Module->prototype(str::u8::Build(u8"mem_lookup_read_", i, u8"_type"),
				{ { u8"address", wasm::Type::i64 }, { u8"access", wasm::Type::i64 }, { u8"cache", wasm::Type::i32 } }, { type });

			/* setup the read-function */
			state.reads[i] = fMakeReadLookup(size, true, static_cast<gen::MemoryType>(i), readPrototype, readWrapper);

			/* setup the code-function */
			if (codeIndex != 4)
				codeLookup[codeIndex] = fMakeReadLookup(size, false, static_cast<gen::MemoryType>(i), readPrototype, readWrapper);

			/* setup the write-prototype */
			std::vector<wasm::Param> writeParams{ { u8"address", wasm::Type::i64 }, { u8"access", wasm::Type::i64 }, { u8"cache", wasm::Type::i32 }, { u8"value", type } };
			if (env::Instance()->detectWriteExecute())
				writeParams.push_back({ u8"inst_size", wasm::Type::i32 });
			wasm::Prototype writePrototype = gen::Module->prototype(str::u8::Build(u8"mem_lookup_write_", i, u8"_type"), writeParams, {});

			/* create the write-function and add the implementation */
			state.writes[i] = fMakeWriteLookup(size, static_cast<gen::MemoryType>(i), writePrototype, writeWrapper);
		}
	}

	/* add the write-to-physical function */
	{
		wasm::Prototype prototype = gen::Module->prototype(u8"mem_write_to_physical_type", { { u8"dest", wasm::Type::i32 }, { u8"source", wasm::Type::i32 }, { u8"size", wasm::Type::i32 } }, {});
		wasm::Sink sink{ gen::Module->function(u8"mem_write_to_physical", prototype, wasm::Export{}) };

		/* setup the copy operation and perform it */
		sink[I::Param::Get(0)];
		sink[I::Param::Get(1)];
		sink[I::Param::Get(2)];
		sink[I::Memory::Copy(physical, memory)];
	}

	/* add the read-from-physical function */
	{
		wasm::Prototype prototype = gen::Module->prototype(u8"mem_read_from_physical_type", { { u8"dest", wasm::Type::i32 }, { u8"source", wasm::Type::i32 }, { u8"size", wasm::Type::i32 } }, {});
		wasm::Sink sink{ gen::Module->function(u8"mem_read_from_physical", prototype, wasm::Export{}) };

		/* setup the copy operation and perform it */
		sink[I::Param::Get(0)];
		sink[I::Param::Get(1)];
		sink[I::Param::Get(2)];
		sink[I::Memory::Copy(memory, physical)];
	}

	/* add the clear-physical function */
	{
		wasm::Prototype prototype = gen::Module->prototype(u8"mem_clear_physical_type", { { u8"dest", wasm::Type::i32 }, { u8"size", wasm::Type::i32 } }, {});
		wasm::Sink sink{ gen::Module->function(u8"mem_clear_physical", prototype, wasm::Export{}) };

		/* setup the clear operation and perform it */
		sink[I::Param::Get(0)];
		sink[I::U32::Const(0)];
		sink[I::Param::Get(1)];
		sink[I::Memory::Fill(physical)];
	}

	/* add the memory-expansion function */
	{
		wasm::Prototype prototype = gen::Module->prototype(u8"mem_expand_physical_type", { { u8"pages", wasm::Type::i32 } }, { wasm::Type::i32 });
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
		wasm::Prototype prototype = gen::Module->prototype(u8"mem_read", { { u8"address", wasm::Type::i64 }, { u8"size", wasm::Type::i32 } }, { wasm::Type::i64 });
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
		_writer.fMakeRead(env::detail::MemoryAccess::ReadCache(), gen::MemoryType::u8To64, env::detail::MainAccessAddress, 0);
		sink[I::Return()];

		_block2.close();
		_writer.fMakeRead(env::detail::MemoryAccess::ReadCache(), gen::MemoryType::u16To64, env::detail::MainAccessAddress, 0);
		sink[I::Return()];

		_block4.close();
		_writer.fMakeRead(env::detail::MemoryAccess::ReadCache(), gen::MemoryType::u32To64, env::detail::MainAccessAddress, 0);
		sink[I::Return()];

		_block8.close();
		_writer.fMakeRead(env::detail::MemoryAccess::ReadCache(), gen::MemoryType::i64, env::detail::MainAccessAddress, 0);

		/* clear the sink reference */
		gen::Instance()->setSink(0);
	}

	/* add the write-access function */
	{
		wasm::Prototype prototype = gen::Module->prototype(u8"mem_write", { { u8"address", wasm::Type::i64 }, { u8"size", wasm::Type::i32 }, { u8"value", wasm::Type::i64 } }, {});
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
		_writer.fMakeWrite(env::detail::MemoryAccess::WriteCache(), gen::MemoryType::u8To64, env::detail::MainAccessAddress, env::detail::MainAccessAddress);
		sink[I::Return()];

		_block2.close();
		_writer.fMakeWrite(env::detail::MemoryAccess::WriteCache(), gen::MemoryType::u16To64, env::detail::MainAccessAddress, env::detail::MainAccessAddress);
		sink[I::Return()];

		_block4.close();
		_writer.fMakeWrite(env::detail::MemoryAccess::WriteCache(), gen::MemoryType::u32To64, env::detail::MainAccessAddress, env::detail::MainAccessAddress);
		sink[I::Return()];

		_block8.close();
		_writer.fMakeWrite(env::detail::MemoryAccess::WriteCache(), gen::MemoryType::i64, env::detail::MainAccessAddress, env::detail::MainAccessAddress);

		/* clear the sink reference */
		gen::Instance()->setSink(0);
	}

	/* add the code-access function */
	{
		wasm::Prototype prototype = gen::Module->prototype(u8"mem_code", { { u8"address", wasm::Type::i64 }, { u8"size", wasm::Type::i32 } }, { wasm::Type::i64 });
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
		_writer.fMakeRead(env::detail::MemoryAccess::CodeCache(), gen::MemoryType::u8To64, env::detail::MainAccessAddress, codeLookup + 0);
		sink[I::Return()];

		_block2.close();
		_writer.fMakeRead(env::detail::MemoryAccess::CodeCache(), gen::MemoryType::u16To64, env::detail::MainAccessAddress, codeLookup + 1);
		sink[I::Return()];

		_block4.close();
		_writer.fMakeRead(env::detail::MemoryAccess::CodeCache(), gen::MemoryType::u32To64, env::detail::MainAccessAddress, codeLookup + 2);
		sink[I::Return()];

		_block8.close();
		_writer.fMakeRead(env::detail::MemoryAccess::CodeCache(), gen::MemoryType::i64, env::detail::MainAccessAddress, codeLookup + 3);

		/* clear the sink reference */
		gen::Instance()->setSink(0);
	}
}
void gen::detail::MemoryBuilder::setupBlockImports(const wasm::Memory& memory, const wasm::Memory& physical, detail::MemoryState& state) const {
	state.memory = memory;
	state.physical = physical;

	/* setup the actual function imports to the lookup functions */
	size_t caches = env::detail::MemoryAccess::CacheCount();
	for (size_t i = 0; i < size_t(gen::MemoryType::_end); ++i) {
		/* resolve the wasm-type */
		wasm::Type type = wasm::Type::i32;
		switch (static_cast<gen::MemoryType>(i)) {
		case gen::MemoryType::u8To64:
		case gen::MemoryType::u16To64:
		case gen::MemoryType::u32To64:
		case gen::MemoryType::i8To64:
		case gen::MemoryType::i16To64:
		case gen::MemoryType::i32To64:
		case gen::MemoryType::i64:
			type = wasm::Type::i64;
			break;
		case gen::MemoryType::f32:
			type = wasm::Type::f32;
			break;
		case gen::MemoryType::f64:
			type = wasm::Type::f64;
			break;
		case gen::MemoryType::u8To32:
		case gen::MemoryType::u16To32:
		case gen::MemoryType::i8To32:
		case gen::MemoryType::i16To32:
		case gen::MemoryType::i32:
		default:
			break;
		}

		/* setup the prototypes */
		wasm::Prototype readPrototype = gen::Module->prototype(str::u8::Build(u8"mem_lookup_read_", i, u8"_type"),
			{ { u8"address", wasm::Type::i64 }, { u8"access", wasm::Type::i64 }, { u8"cache", wasm::Type::i32 } }, { type });

		std::vector<wasm::Param> writeParams{ { u8"address", wasm::Type::i64 }, { u8"access", wasm::Type::i64 }, { u8"cache", wasm::Type::i32 }, { u8"value", type } };
		if (env::Instance()->detectWriteExecute())
			writeParams.push_back({ u8"inst_size", wasm::Type::i32 });
		wasm::Prototype writePrototype = gen::Module->prototype(str::u8::Build(u8"mem_lookup_write_", i, u8"_type"), writeParams, {});

		/* add the imports */
		state.reads[i] = gen::Module->function(str::u8::Build(u8"mem_lookup_read_", i), readPrototype, wasm::Import{ u8"mem" });
		state.writes[i] = gen::Module->function(str::u8::Build(u8"mem_lookup_write_", i), writePrototype, wasm::Import{ u8"mem" });
	}
}
