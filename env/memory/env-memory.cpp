#include "env-memory.h"
#include "bridge-memory.h"

namespace I = wasm::inst;

env::Memory::Memory(env::Context& context, uint32_t cacheSize) : pContext{ &context }, pCacheCount{ cacheSize } {
	pReadCache = pCacheCount + 0;
	pWriteCache = pCacheCount + 1;
	pExecuteCache = pCacheCount + 2;
	pCachePages = env::PageCount(uint64_t(pCacheCount + 3) * sizeof(Memory::MemCache));
}

void env::Memory::fCheckCache(uint32_t cache) const {
	if (cache >= pCacheCount)
		pContext->fail(u8"Cache [", cache, u8"] out of bounds as only [", pCacheCount, u8"] caches have been defined");
}
void env::Memory::fMakeAddress(wasm::Sink& sink, const env::MemoryState& state, uint32_t cache, const wasm::Variable& i64Address, const wasm::Function& lookup, env::MemoryType type) const {
	/* compute the offset into the current cached region */
	sink[I::Local::Get(i64Address)];
	sink[I::U32::Const(cache * sizeof(Memory::MemCache))];
	sink[I::U64::Load(state.caches, offsetof(Memory::MemCache, address))];
	sink[I::U64::Sub()];
	sink[I::Local::Tee(i64Address)];

	/* check if the address lies in the range */
	sink[I::U32::Const(cache * sizeof(Memory::MemCache))];
	switch (type) {
	case env::MemoryType::u8To32:
	case env::MemoryType::u8To64:
	case env::MemoryType::i8To32:
	case env::MemoryType::i8To64:
		sink[I::U64::Load32(state.caches, offsetof(Memory::MemCache, size1))];
		break;
	case env::MemoryType::u16To32:
	case env::MemoryType::u16To64:
	case env::MemoryType::i16To32:
	case env::MemoryType::i16To64:
		sink[I::U64::Load32(state.caches, offsetof(Memory::MemCache, size2))];
		break;
	case env::MemoryType::i32:
	case env::MemoryType::f32:
	case env::MemoryType::u32To64:
	case env::MemoryType::i32To64:
		sink[I::U64::Load32(state.caches, offsetof(Memory::MemCache, size4))];
		break;
	case env::MemoryType::i64:
	case env::MemoryType::f64:
		sink[I::U64::Load32(state.caches, offsetof(Memory::MemCache, size8))];
		break;
	}
	sink[I::U64::GreaterEqual()];

	{
		/* greater-equal, perform mem-cache lookup */
		wasm::IfThen _if{ sink, {}, state.ifElsePrototype };

		/* recover the original address */
		sink[I::Local::Get(i64Address)];
		sink[I::U32::Const(cache * sizeof(Memory::MemCache))];
		sink[I::U64::Load(state.caches, offsetof(Memory::MemCache, address))];
		sink[I::U64::Add()];

		/* write the current size to the stack */
		switch (type) {
		case env::MemoryType::u8To32:
		case env::MemoryType::u8To64:
		case env::MemoryType::i8To32:
		case env::MemoryType::i8To64:
			sink[I::U32::Const(1)];
			break;
		case env::MemoryType::u16To32:
		case env::MemoryType::u16To64:
		case env::MemoryType::i16To32:
		case env::MemoryType::i16To64:
			sink[I::U32::Const(2)];
			break;
		case env::MemoryType::i32:
		case env::MemoryType::f32:
		case env::MemoryType::u32To64:
		case env::MemoryType::i32To64:
			sink[I::U32::Const(4)];
			break;
		case env::MemoryType::i64:
		case env::MemoryType::f64:
			sink[I::U32::Const(8)];
			break;
		}

		/* perform the call to patch the cache (leaves the new absolute address as i32 on the stack) */
		sink[I::U32::Const(cache * sizeof(Memory::MemCache))];
		sink[I::Call::Direct(lookup)];

		/* less-equal, compute the final absolute address */
		_if.otherwise();
		sink[I::Local::Get(i64Address)];
		sink[I::U64::Shrink()];
		sink[I::U32::Const(cache * sizeof(Memory::MemCache))];
		sink[I::U32::Load(state.caches, offsetof(Memory::MemCache, physical))];
		sink[I::U32::Add()];
	}
}
void env::Memory::fMakeLookup(const wasm::Memory& caches, const wasm::Function& function, const wasm::Function& lookup, const wasm::Function& lookupPhysical, const wasm::Function& lookupSize, uint32_t uasge) const {
	wasm::Sink sink{ function };
	wasm::Variable address = sink.parameter(0), size = sink.parameter(1), cache = sink.parameter(2);

	/* allocate the local variables necessary to write the temporary result back */
	wasm::Variable outAddr = sink.local(wasm::Type::i64, u8"out_address");
	wasm::Variable outPhys = sink.local(wasm::Type::i32, u8"out_physical");
	wasm::Variable outSize = sink.local(wasm::Type::i32, u8"out_size");

	/* perform the call (only returns the address) */
	sink[I::U64::Const(this)];
	sink[I::Local::Get(address)];
	sink[I::Local::Get(size)];
	sink[I::U32::Const(uasge)];
	sink[I::Call::Direct(lookup)];
	sink[I::Local::Set(outAddr)];

	/* fetch the remainder of the results */
	sink[I::U64::Const(this)];
	sink[I::Call::Direct(lookupPhysical)];
	sink[I::Local::Set(outPhys)];
	sink[I::U64::Const(this)];
	sink[I::Call::Direct(lookupSize)];
	sink[I::Local::Set(outSize)];

	/* write the address and physical offset back */
	sink[I::Local::Get(cache)];
	sink[I::Local::Get(outAddr)];
	sink[I::U64::Store(caches, offsetof(Memory::MemCache, address))];
	sink[I::Local::Get(cache)];
	sink[I::Local::Get(outPhys)];
	sink[I::U32::Store(caches, offsetof(Memory::MemCache, physical))];

	/* write the different sizes back */
	for (uint32_t i = 1; i <= 8; i *= 2) {
		sink[I::Local::Get(cache)];
		sink[I::Local::Get(outSize)];
		sink[I::U32::Const(i)];
		sink[I::U32::Sub()];
		switch (i) {
		case 1:
			sink[I::U32::Store(caches, offsetof(Memory::MemCache, size1))];
			break;
		case 2:
			sink[I::U32::Store(caches, offsetof(Memory::MemCache, size2))];
			break;
		case 4:
			sink[I::U32::Store(caches, offsetof(Memory::MemCache, size4))];
			break;
		case 8:
			sink[I::U32::Store(caches, offsetof(Memory::MemCache, size8))];
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
void env::Memory::fMakeRead(wasm::Sink& sink, const wasm::Variable& i64Address, const env::MemoryState& state, uint32_t cache, env::MemoryType type) const {
	fMakeAddress(sink, state, cache, i64Address, state.readFunction, type);

	/* add the final read-instruction */
	switch (type) {
	case env::MemoryType::u8To32:
		sink[I::U32::Load8(state.memory)];
		break;
	case env::MemoryType::u16To32:
		sink[I::U32::Load16(state.memory)];
		break;
	case env::MemoryType::u8To64:
		sink[I::U64::Load8(state.memory)];
		break;
	case env::MemoryType::u16To64:
		sink[I::U64::Load16(state.memory)];
		break;
	case env::MemoryType::u32To64:
		sink[I::U64::Load32(state.memory)];
		break;
	case env::MemoryType::i8To32:
		sink[I::I32::Load8(state.memory)];
		break;
	case env::MemoryType::i16To32:
		sink[I::I32::Load16(state.memory)];
		break;
	case env::MemoryType::i8To64:
		sink[I::I64::Load8(state.memory)];
		break;
	case env::MemoryType::i16To64:
		sink[I::I64::Load16(state.memory)];
		break;
	case env::MemoryType::i32To64:
		sink[I::I64::Load32(state.memory)];
		break;
	case env::MemoryType::i32:
		sink[I::U32::Load(state.memory)];
		break;
	case env::MemoryType::i64:
		sink[I::U64::Load(state.memory)];
		break;
	case env::MemoryType::f32:
		sink[I::F32::Load(state.memory)];
		break;
	case env::MemoryType::f64:
		sink[I::F64::Load(state.memory)];
		break;
	}
}
void env::Memory::fMakeWrite(wasm::Sink& sink, const wasm::Variable& i64Address, const wasm::Variable& value, const env::MemoryState& state, uint32_t cache, env::MemoryType type) const {
	fMakeAddress(sink, state, cache, i64Address, state.writeFunction, type);

	/* write the value to the stack */
	sink[I::Local::Get(value)];

	/* add the final store-instruction */
	switch (type) {
	case env::MemoryType::u8To32:
	case env::MemoryType::i8To32:
		sink[I::U32::Store8(state.memory)];
		break;
	case env::MemoryType::u16To32:
	case env::MemoryType::i16To32:
		sink[I::U32::Store16(state.memory)];
		break;
	case env::MemoryType::u8To64:
	case env::MemoryType::i8To64:
		sink[I::U64::Store8(state.memory)];
		break;
	case env::MemoryType::u16To64:
	case env::MemoryType::i16To64:
		sink[I::U64::Store16(state.memory)];
		break;
	case env::MemoryType::u32To64:
	case env::MemoryType::i32To64:
		sink[I::U64::Store32(state.memory)];
		break;
	case env::MemoryType::i32:
		sink[I::U32::Store(state.memory)];
		break;
	case env::MemoryType::i64:
		sink[I::U64::Store(state.memory)];
		break;
	case env::MemoryType::f32:
		sink[I::F32::Store(state.memory)];
		break;
	case env::MemoryType::f64:
		sink[I::F64::Store(state.memory)];
		break;
	}
}
void env::Memory::fMakeExecute(wasm::Sink& sink, const wasm::Variable& i64Address, const env::MemoryState& state, uint32_t cache, env::MemoryType type) const {
	fMakeAddress(sink, state, cache, i64Address, state.executeFunction, type);

	/* add the final read-instruction */
	switch (type) {
	case env::MemoryType::u8To32:
		sink[I::U32::Load8(state.memory)];
		break;
	case env::MemoryType::u16To32:
		sink[I::U32::Load16(state.memory)];
		break;
	case env::MemoryType::u8To64:
		sink[I::U64::Load8(state.memory)];
		break;
	case env::MemoryType::u16To64:
		sink[I::U64::Load16(state.memory)];
		break;
	case env::MemoryType::u32To64:
		sink[I::U64::Load32(state.memory)];
		break;
	case env::MemoryType::i8To32:
		sink[I::I32::Load8(state.memory)];
		break;
	case env::MemoryType::i16To32:
		sink[I::I32::Load16(state.memory)];
		break;
	case env::MemoryType::i8To64:
		sink[I::I64::Load8(state.memory)];
		break;
	case env::MemoryType::i16To64:
		sink[I::I64::Load16(state.memory)];
		break;
	case env::MemoryType::i32To64:
		sink[I::I64::Load32(state.memory)];
		break;
	case env::MemoryType::i32:
		sink[I::U32::Load(state.memory)];
		break;
	case env::MemoryType::i64:
		sink[I::U64::Load(state.memory)];
		break;
	case env::MemoryType::f32:
		sink[I::F32::Load(state.memory)];
		break;
	case env::MemoryType::f64:
		sink[I::F64::Load(state.memory)];
		break;
	}
}
void env::Memory::fMakeAccess(wasm::Module& mod, const env::MemoryState& state, const wasm::Prototype& readPrototype, const wasm::Prototype& writePrototype, std::u8string_view name, env::MemoryType type) const {
	wasm::Sink read{ mod.function(str::Build<std::u8string>(u8"mem_read_", name), readPrototype, {}, true) };
	wasm::Sink write{ mod.function(str::Build<std::u8string>(u8"mem_write_", name), writePrototype, {}, true) };
	wasm::Sink execute{ mod.function(str::Build<std::u8string>(u8"mem_execute_", name), readPrototype, {}, true) };

	/* make the read, write, and execute function */
	fMakeRead(read, read.parameter(0), state, pReadCache, type);
	fMakeExecute(execute, execute.parameter(0), state, pExecuteCache, type);
	fMakeWrite(write, write.parameter(0), write.parameter(1), state, pExecuteCache, type);
}

size_t env::Memory::fLookupVirtual(env::addr_t address) const {
	size_t begin = 0, end = pVirtual.size();
	while ((end - begin) > 1) {
		size_t index = (begin + end + 1) / 2;
		if (address < pVirtual[index].address)
			end = index;
		else
			begin = index;
	}
	return begin;
}
size_t env::Memory::fLookupPhysical(env::physical_t physical) const {
	size_t begin = 0, end = pPhysical.size();
	while ((end - begin) > 1) {
		size_t index = (begin + end + 1) / 2;
		if (physical < pPhysical[index].physical)
			end = index;
		else
			begin = index;
	}
	return begin;
}
void env::Memory::fLookup(env::addr_t address, uint32_t size, uint32_t usage) const {
	util::log(str::Format<std::u8string>(u8"Lookup [{:#16x}] with size [{}] and usage [{:04b}]", address, size, usage));

	/* lookup the virtual mapping containing the corresponding address */
	size_t index = fLookupVirtual(address);

	/* check if an entry has been found */
	if (index >= pVirtual.size() || address < pVirtual[index].address || address >= pVirtual[index].address + pVirtual[index].size)
		util::fail(str::Format<std::u8string>(u8"Virtual page-fault at address [{:#16x}] encountered", address));

	/* check if the usage attributes are valid */
	if ((pVirtual[index].usage & usage) != usage)
		util::fail(str::Format<std::u8string>(u8"Virtual page-protection fault at address [{:#16x}] encountered", address));

	/* collect all previous and upcoming regions of the same usage */
	pLastLookup = Memory::MemLookup{ pVirtual[index].address, pVirtual[index].physical, pVirtual[index].size };
	for (size_t i = index; i > 0; --i) {
		if (pVirtual[i - 1].address + pVirtual[i - 1].size != pVirtual[i].address)
			break;
		if ((pVirtual[i - 1].usage & usage) != usage)
			break;
		pLastLookup = { pVirtual[i - 1].address, pVirtual[i - 1].physical, pVirtual[index].size + pLastLookup.size };
	}
	for (size_t i = index + 1; i < pVirtual.size(); ++i) {
		if (pLastLookup.address + pLastLookup.size != pVirtual[i].address)
			break;
		if ((pVirtual[i].usage & usage) != usage)
			break;
		pLastLookup.size += pVirtual[i].size;
	}

	/* check if the access-size is valid */
	if (pLastLookup.address + pLastLookup.size < address + size)
		util::fail(str::Format<std::u8string>(u8"Virtual page-fault at address [{:#16x}] encountered", address));
}
bool env::Memory::fMemMap(env::addr_t address, uint32_t size, uint32_t usage) {
	/* check if the address and size are aligned properly and validate the usage */
	if (env::PageOffset(address) != 0 || env::PageOffset(size) != 0)
		util::fail(u8"Memory-mapping requires address and size to be page-aligned");
	if ((usage & ~(env::MemoryUsage::Read | env::MemoryUsage::Write | env::MemoryUsage::Execute)) != 0)
		util::fail(u8"Memory-usage must only consist of read/write/execute usage");

	/* check if the given range is valid and does not overlap any existing ranges */
	size_t index = fLookupVirtual(address);
	if (index < pVirtual.size() && address >= pVirtual[index].address && address < pVirtual[index].address + pVirtual[index].size)
		util::fail(u8"Memory-mapping address overlaps existing mappings");
	if (index + 1 < pVirtual.size() && address + size >= pVirtual[index + 1].address)
		util::fail(u8"Memory-mapping address overlaps existing mappings");

	/* check if the previous or neighboring virtual page is already allocated */
	bool hasPrevious = (pVirtual[index].address + pVirtual[index].size == address);
	bool hasNext = (index + 1 < pVirtual.size() && pVirtual[index + 1].address + pVirtual[index + 1].size == address);

	/* check if the region can just be expanded (while to allow breaking out) */
	while (hasPrevious && !hasNext) {
		size_t physical = fLookupPhysical(pVirtual[index].physical);
		if (physical + 1 < pPhysical.size()) {
			if (pPhysical[physical + 1].used)
				break;
			if (pPhysical[physical + 1].size < size && physical + 2 < pPhysical.size())
				break;
		}

		/* check if the next physical page can be consumed */
		if (physical + 1 < pPhysical.size() && pPhysical[physical + 1].size >= size) {
			pPhysical[physical].size += size;

			/* either reduce the next physical slot or remove it entirely if consumed */
			if (size > pPhysical[physical + 1].size)
				pPhysical[physical + 1].size -= size;
			else
				pPhysical.erase(pPhysical.begin() + physical + 1);

			/* flush the caches to ensure the new mapping is applied */
			fFlushCaches();
			return true;
		}

		/* compute the number of physical pages to allocate (allocate an overhead to add a potential buffer) */
		uint32_t needed = size - (physical + 1 < pPhysical.size() ? pPhysical[physical + 1].size : 0);
		uint32_t allocate = std::min<uint32_t>(env::MinUpscalePages, needed + size);
		if (!fExpandPhysical(allocate))
			allocate = needed;
		if (!fExpandPhysical(allocate))
			return false;

		/* update the virtual slot and the current physical page slot */
		if (pVirtual[index].usage == usage)
			pVirtual[index].size += size;
		else
			pVirtual.insert(pVirtual.begin() + index + 1, Memory::MemVirtual{ address, pPhysical[physical].physical + pPhysical[physical].size, size, usage });
		pPhysical[physical].size += size;

		/* write the remaining data to the next physical slot */
		if (allocate > needed) {
			if (physical + 1 >= pPhysical.size())
				pPhysical.emplace_back().used = false;
			else
				pPhysical[physical + 1].size -= size;
			pPhysical[physical + 1].physical = pPhysical[physical].physical + pPhysical[physical].size;
			pPhysical[physical + 1].size += allocate - needed;
		}
		else if (physical + 1 >= pPhysical.size())
			pPhysical.pop_back();

		/* flush the caches to ensure the new mapping is applied */
		fFlushCaches();
		return true;
	}

	/* look for the next region large enough to store the required memory */
	uint32_t total = (hasPrevious ? pVirtual[index].size : 0) + size + (hasNext ? pVirtual[index + 1].size : 0);
	size_t physical = 0;
	for (; physical < pPhysical.size(); ++physical) {
		if (!pPhysical[physical].used && pPhysical[physical].size >= total)
			break;
	}

	/* check if the physical memory needs to be allocated */
	if (physical == pPhysical.size()) {
		/* allocate the necessary memory (more than necessary to allow upscaling) */
		uint32_t allocate = std::min<uint32_t>(env::MinUpscalePages, total + size);
		if (!fExpandPhysical(allocate))
			allocate = size;
		if (!fExpandPhysical(allocate))
			return false;

		/* add the physical slot */
		pPhysical.push_back({ (pPhysical.empty() ? 0 : pPhysical.back().physical), allocate, false });
	}

	/* move the original regions into place */
	env::physical_t actual = pPhysical[physical].physical;
	if (hasPrevious) {
		fMovePhysical(pPhysical[physical].physical, pVirtual[index].physical, pVirtual[index].size);
		pPhysical[fLookupPhysical(pVirtual[index].physical)].used = false;
		actual += pVirtual[index].size;
	}
	if (hasNext) {
		fMovePhysical(pPhysical[physical].physical + (total - pVirtual[index + 1].size), pVirtual[index + 1].physical, pVirtual[index + 1].size);
		pPhysical[fLookupPhysical(pVirtual[index + 1].physical)].used = false;
	}

	/* mark the physical slot as used and move the remainder into a new slot */
	uint32_t remainder = (pPhysical[physical].size - total);
	pPhysical[physical].used = true;
	pPhysical[physical].size = total;
	if (remainder > 0) {
		if (physical + 1 >= pPhysical.size())
			pPhysical.push_back({ pPhysical[physical].physical + total, remainder, false });
		else if (!pPhysical[physical + 1].used)
			pPhysical[physical + 1] = { pPhysical[physical].physical + total, pPhysical[physical + 1].size + remainder, false };
		else
			pPhysical.insert(pPhysical.begin() + physical + 1, { pPhysical[physical].physical + total, remainder, false });
	}

	/* check if the virtual entry can be merged with the next entry, and potential the previous entry as well */
	if (hasNext && pVirtual[index + 1].usage == usage) {
		pVirtual[index + 1] = { address, actual, pVirtual[index + 1].size + size, usage };

		/* check if it can be merged into the previous slot as well */
		if (hasPrevious && pVirtual[index].usage == usage) {
			pVirtual[index].size += pVirtual[index + 1].size;
			pVirtual.erase(pVirtual.begin() + index + 1);
		}
	}

	/* check if the virtual entry can be merged with the previous entry */
	else if (hasPrevious && pVirtual[index].usage == usage)
		pVirtual[index].size += size;
	else
		pVirtual.insert(pVirtual.begin() + (++index), { address, actual, size, usage });

	/* merge neighboring unused slots */
	if (hasPrevious || hasNext) {
		size_t dropped = 0;
		for (size_t i = 1; i < pPhysical.size(); ++i) {
			if (!pPhysical[i - dropped - 1].used && !pPhysical[i].used) {
				++dropped;
				pPhysical[i - dropped].size += pPhysical[i].size;
			}
			else if (dropped > 0)
				pPhysical[i - dropped] = pPhysical[i];
		}
		if (dropped > 0)
			pPhysical.resize(pPhysical.size() - dropped);
	}

	/* flush the caches to ensure the new mapping is accepted */
	fFlushCaches();
	return true;
}

const env::Memory::MemLookup& env::Memory::fLastLookup() const {
	return pLastLookup;
}
bool env::Memory::fExpandPhysical(uint32_t size) const {
	return bridge::Memory::ExpandPhysical(pContext->id(), size);
}
void env::Memory::fMovePhysical(env::physical_t dest, env::physical_t source, uint32_t size) const {
	bridge::Memory::MovePhysical(pContext->id(), dest, source, size);
}
void env::Memory::fFlushCaches() const {
	bridge::Memory::FlushCaches(pContext->id());
}
uint32_t env::Memory::fReadi32Fromi8(env::addr_t address) const {
	return bridge::Memory::Readi32Fromi8(pContext->id(), address);
}
uint32_t env::Memory::fReadi32Fromu8(env::addr_t address) const {
	return bridge::Memory::Readi32Fromu8(pContext->id(), address);
}
uint32_t env::Memory::fReadi32Fromi16(env::addr_t address) const {
	return bridge::Memory::Readi32Fromi16(pContext->id(), address);
}
uint32_t env::Memory::fReadi32Fromu16(env::addr_t address) const {
	return bridge::Memory::Readi32Fromu16(pContext->id(), address);
}
uint32_t env::Memory::fReadi32(env::addr_t address) const {
	return bridge::Memory::Readi32(pContext->id(), address);
}
uint64_t env::Memory::fReadi64(env::addr_t address) const {
	return bridge::Memory::Readi64(pContext->id(), address);
}
float env::Memory::fReadf32(env::addr_t address) const {
	return bridge::Memory::Readf32(pContext->id(), address);
}
double env::Memory::fReadf64(env::addr_t address) const {
	return bridge::Memory::Readf64(pContext->id(), address);
}
void env::Memory::fWritei32Fromi8(env::addr_t address, uint32_t value) const {
	bridge::Memory::Writei32Fromi8(pContext->id(), address, value);
}
void env::Memory::fWritei32Fromu8(env::addr_t address, uint32_t value) const {
	bridge::Memory::Writei32(pContext->id(), address, value);
}
void env::Memory::fWritei32Fromi16(env::addr_t address, uint32_t value) const {
	bridge::Memory::Writei32Fromi16(pContext->id(), address, value);
}
void env::Memory::fWritei32Fromu16(env::addr_t address, uint32_t value) const {
	bridge::Memory::Writei32Fromu16(pContext->id(), address, value);
}
void env::Memory::fWritei32(env::addr_t address, uint32_t value) const {
	bridge::Memory::Writei32(pContext->id(), address, value);
}
void env::Memory::fWritei64(env::addr_t address, uint64_t value) const {
	bridge::Memory::Writei64(pContext->id(), address, value);
}
void env::Memory::fWritef32(env::addr_t address, float value) const {
	bridge::Memory::Writef32(pContext->id(), address, value);
}
void env::Memory::fWritef64(env::addr_t address, double value) const {
	bridge::Memory::Writef64(pContext->id(), address, value);
}
uint32_t env::Memory::fExecutei32Fromi8(env::addr_t address) const {
	return bridge::Memory::Executei32Fromi8(pContext->id(), address);
}
uint32_t env::Memory::fExecutei32Fromu8(env::addr_t address) const {
	return bridge::Memory::Executei32Fromu8(pContext->id(), address);
}
uint32_t env::Memory::fExecutei32Fromi16(env::addr_t address) const {
	return bridge::Memory::Executei32Fromi16(pContext->id(), address);
}
uint32_t env::Memory::fExecutei32Fromu16(env::addr_t address) const {
	return bridge::Memory::Executei32Fromu16(pContext->id(), address);
}
uint32_t env::Memory::fExecutei32(env::addr_t address) const {
	return bridge::Memory::Executei32(pContext->id(), address);
}
uint64_t env::Memory::fExecutei64(env::addr_t address) const {
	return bridge::Memory::Executei64(pContext->id(), address);
}
float env::Memory::fExecutef32(env::addr_t address) const {
	return bridge::Memory::Executef32(pContext->id(), address);
}
double env::Memory::fExecutef64(env::addr_t address) const {
	return bridge::Memory::Executef64(pContext->id(), address);
}

env::MemoryState env::Memory::setupCoreModule(wasm::Module& mod) const {
	env::MemoryState state;

	/* add the import to the lookup-function */
	wasm::Prototype lookupPrototype = mod.prototype(u8"mem_lookup_type",
		{ { u8"self", wasm::Type::i64 }, { u8"addr", wasm::Type::i64 }, { u8"size", wasm::Type::i32 }, { u8"usage", wasm::Type::i32 } },
		{ wasm::Type::i64 }
	);
	wasm::Prototype resultPrototype = mod.prototype(u8"mem_lookup_result_type", { { u8"self", wasm::Type::i64 } }, { wasm::Type::i32 });
	wasm::Function lookup = mod.function(u8"mem_import_lookup", lookupPrototype, u8"memory");
	wasm::Function lookupPhysical = mod.function(u8"mem_import_lookup_physical", resultPrototype, u8"memory");
	wasm::Function lookupSize = mod.function(u8"mem_import_lookup_size", resultPrototype, u8"memory");

	/* add the import to mmap */
	wasm::Prototype mmapType = mod.prototype(u8"mem_mmap_type",
		{ { u8"self", wasm::Type::i64 }, { u8"addr", wasm::Type::i64 }, { u8"size", wasm::Type::i32 }, { u8"usage", wasm::Type::i32 } },
		{ wasm::Type::i32 }
	);
	state.mmapFunction = mod.function(u8"mem_mmap", mmapType, u8"memory", true);

	/* add the core linear memory and page-lookup */
	state.memory = mod.memory(u8"mem_core", wasm::Limit{ env::MinPages }, {}, true);
	state.caches = mod.memory(u8"mem_cache", wasm::Limit{ pCachePages, pCachePages }, {}, true);

	/* add the functions for the page-patching (receive the address as parameter and return the new absolute address) */
	wasm::Prototype prototype = mod.prototype(u8"mem_addr_lookup", { { u8"addr", wasm::Type::i64 }, { u8"size", wasm::Type::i32 }, { u8"cache", wasm::Type::i32 } }, { wasm::Type::i32 });
	state.readFunction = mod.function(u8"mem_lookup_read", prototype, {}, true);
	state.writeFunction = mod.function(u8"mem_lookup_write", prototype, {}, true);
	state.executeFunction = mod.function(u8"mem_lookup_execute", prototype, {}, true);

	/* add the actual implementations */
	fMakeLookup(state.caches, state.readFunction, lookup, lookupPhysical, lookupSize, env::MemoryUsage::Read);
	fMakeLookup(state.caches, state.writeFunction, lookup, lookupPhysical, lookupSize, env::MemoryUsage::Write);
	fMakeLookup(state.caches, state.executeFunction, lookup, lookupPhysical, lookupSize, env::MemoryUsage::Execute);

	/* add the if-else prototype */
	state.ifElsePrototype = mod.prototype(u8"mem_if_else", {}, { wasm::Type::i32 });

	/* add the separate access functions */
	wasm::Prototype readi32 = mod.prototype(u8"mem_read_i32", { { u8"addr", wasm::Type::i64 } }, { wasm::Type::i32 });
	wasm::Prototype readf32 = mod.prototype(u8"mem_read_f32", { { u8"addr", wasm::Type::i64 } }, { wasm::Type::f32 });
	wasm::Prototype readi64 = mod.prototype(u8"mem_read_i64", { { u8"addr", wasm::Type::i64 } }, { wasm::Type::i64 });
	wasm::Prototype readf64 = mod.prototype(u8"mem_read_f64", { { u8"addr", wasm::Type::i64 } }, { wasm::Type::f64 });
	wasm::Prototype writei32 = mod.prototype(u8"mem_write_i32", { { u8"addr", wasm::Type::i64 }, { u8"value", wasm::Type::i32 } }, {});
	wasm::Prototype writef32 = mod.prototype(u8"mem_write_f32", { { u8"addr", wasm::Type::i64 }, { u8"value", wasm::Type::f32 } }, {});
	wasm::Prototype writei64 = mod.prototype(u8"mem_write_i64", { { u8"addr", wasm::Type::i64 }, { u8"value", wasm::Type::i64 } }, {});
	wasm::Prototype writef64 = mod.prototype(u8"mem_write_f64", { { u8"addr", wasm::Type::i64 }, { u8"value", wasm::Type::f64 } }, {});
	fMakeAccess(mod, state, readi32, writei32, u8"u8_i32", env::MemoryType::u8To32);
	fMakeAccess(mod, state, readi32, writei32, u8"i8_i32", env::MemoryType::i8To32);
	fMakeAccess(mod, state, readi32, writei32, u8"u16_i32", env::MemoryType::u16To32);
	fMakeAccess(mod, state, readi32, writei32, u8"i16_i32", env::MemoryType::i16To32);
	fMakeAccess(mod, state, readi32, writei32, u8"i32", env::MemoryType::i32);
	fMakeAccess(mod, state, readi64, writei64, u8"i64", env::MemoryType::i64);
	fMakeAccess(mod, state, readf32, writef32, u8"f32", env::MemoryType::f32);
	fMakeAccess(mod, state, readf64, writef64, u8"f64", env::MemoryType::f64);

	/* add the memory-expansion function */
	wasm::Prototype expandPhysicalType = mod.prototype(u8"mem_expand_physical_type", { { u8"size", wasm::Type::i32 } }, { wasm::Type::i32 });
	{
		wasm::Sink sink{ mod.function(u8"mem_expand_physical", expandPhysicalType, {}, true) };

		/* number of pages to grow by */
		sink[I::Local::Get(sink.parameter(0))];
		sink[I::U32::Const(env::PageSize)];
		sink[I::U32::Div()];

		sink[I::Memory::Grow(state.memory)];

		/* convert result to 1 or 0 */
		sink[I::I32::Const(0)];
		sink[I::I32::Less()];
		{
			wasm::IfThen _if{ sink, {}, state.ifElsePrototype };
			sink[I::U32::Const(0)];
			_if.otherwise();
			sink[I::U32::Const(1)];
		}
	}

	/* add the memory-move function */
	wasm::Prototype movePhysicalType = mod.prototype(u8"mem_move_physical_type", { { u8"dest", wasm::Type::i32 }, { u8"source", wasm::Type::i32 }, { u8"size", wasm::Type::i32 } }, {});
	{
		wasm::Sink sink{ mod.function(u8"mem_move_physical", movePhysicalType, {}, true) };

		/* destination-address */
		sink[I::Local::Get(sink.parameter(0))];

		/* source-address */
		sink[I::Local::Get(sink.parameter(1))];

		/* size */
		sink[I::Local::Get(sink.parameter(2))];

		sink[I::Memory::Copy(state.memory)];
	}

	/* add the cache-flushing function */
	{
		wasm::Sink sink{ mod.function(u8"mem_flush_caches", {}, {}, true) };

		/* destination-address */
		sink[I::U32::Const(0)];

		/* value */
		sink[I::U32::Const(0)];

		/* size */
		sink[I::U32::Const(uint64_t(pCacheCount + 3) * sizeof(Memory::MemCache))];

		sink[I::Memory::Fill(state.caches)];
	}
	return state;
}
env::MemoryState env::Memory::setupImports(wasm::Module& mod) const {
	env::MemoryState state;

	/* add the core linear memory and cache-lookup imports */
	state.memory = mod.memory(u8"mem_core", wasm::Limit{ env::MinPages }, pContext->selfName());
	state.caches = mod.memory(u8"mem_cache", wasm::Limit{ pCachePages, pCachePages }, pContext->selfName());

	/* add the import to mmap */
	wasm::Prototype mmapType = mod.prototype(u8"mem_lookup_type",
		{ { u8"self", wasm::Type::i64 }, { u8"addr", wasm::Type::i64 }, { u8"size", wasm::Type::i32 }, { u8"usage", wasm::Type::i32 } },
		{ wasm::Type::i32 }
	);
	state.mmapFunction = mod.function(u8"mem_mmap", mmapType, pContext->selfName());

	/* add the function-imports for the page-lookup */
	wasm::Prototype prototype = mod.prototype(u8"mem_addr_lookup", { { u8"addr", wasm::Type::i64 }, { u8"size", wasm::Type::i32 }, { u8"cache", wasm::Type::i32 } }, { wasm::Type::i32 });
	state.readFunction = mod.function(u8"mem_lookup_read", prototype, pContext->selfName());
	state.writeFunction = mod.function(u8"mem_lookup_write", prototype, pContext->selfName());
	state.executeFunction = mod.function(u8"mem_lookup_execute", prototype, pContext->selfName());

	/* add the if-else prototype */
	state.ifElsePrototype = mod.prototype(u8"mem_if_else", {}, { wasm::Type::i32 });
	return state;
}
void env::Memory::makeRead(const wasm::Variable& i64Address, const env::MemoryState& state, uint32_t cache, env::MemoryType type) const {
	wasm::Sink& sink = wasm::Variable{ i64Address }.sink();
	fCheckCache(cache);
	return fMakeRead(sink, i64Address, state, cache, type);
}
void env::Memory::makeWrite(const wasm::Variable& i64Address, const wasm::Variable& value, const env::MemoryState& state, uint32_t cache, env::MemoryType type) const {
	wasm::Sink& sink = wasm::Variable{ i64Address }.sink();
	fCheckCache(cache);
	return fMakeWrite(sink, i64Address, value, state, cache, type);
}
void env::Memory::makeExecute(const wasm::Variable& i64Address, const env::MemoryState& state, uint32_t cache, env::MemoryType type) const {
	wasm::Sink& sink = wasm::Variable{ i64Address }.sink();
	fCheckCache(cache);
	return fMakeExecute(sink, i64Address, state, cache, type);
}

bool env::Memory::mmap(env::addr_t address, uint32_t size, uint32_t usage) {
	return fMemMap(address, size, usage);
}
